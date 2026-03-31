use fontdue::{Font, FontSettings};
use std::fs;
use std::path::Path;

/// SDF font generation. Rasterizes at a base size and generates
/// 8-bit signed distance field data. The SDF can be rendered at
/// any target size at runtime with smooth edges.
pub fn convert(
    input: &str,
    base_size: f32,
    spread: f32,
    output_dir: &str,
    name_prefix: &str,
    first_char: u8,
    last_char: u8,
) -> Result<(), String> {
    let font_data = fs::read(input).map_err(|e| format!("Failed to read {input}: {e}"))?;
    let font = Font::from_bytes(font_data, FontSettings::default())
        .map_err(|e| format!("Failed to parse font: {e}"))?;

    fs::create_dir_all(output_dir)
        .map_err(|e| format!("Failed to create output directory: {e}"))?;

    let font_name = font.name().unwrap_or("Unknown");
    let size_int = base_size as u32;
    let var_name = format!("{name_prefix}_sdf{size_int}");
    let filename = format!("{var_name}.hpp");
    let out_path = Path::new(output_dir).join(&filename);

    let num_chars = (last_char - first_char + 1) as usize;

    // Get font metrics
    let line_metrics = font.horizontal_line_metrics(base_size);
    let ascent = if let Some(lm) = line_metrics {
        lm.ascent.ceil() as i32
    } else {
        base_size as i32
    };

    // Rasterize all glyphs and find max dimensions
    let mut max_width = 0usize;
    let mut max_height = 0usize;
    struct GlyphData {
        metrics: fontdue::Metrics,
        bitmap: Vec<u8>, // 8-bit coverage
    }
    let mut glyphs: Vec<GlyphData> = Vec::new();

    for ch in first_char..=last_char {
        let (metrics, bitmap) = font.rasterize(ch as char, base_size);
        if metrics.width > max_width {
            max_width = metrics.width;
        }
        if metrics.height > max_height {
            max_height = metrics.height;
        }
        glyphs.push(GlyphData { metrics, bitmap });
    }

    // Cell size with padding for SDF spread
    let pad = spread.ceil() as usize;
    let cell_w = max_width + pad * 2;
    let cell_h = if let Some(lm) = line_metrics {
        (lm.ascent - lm.descent).ceil() as usize + pad * 2
    } else {
        max_height + pad * 2
    };

    // Generate SDF for each glyph
    let mut sdf_data: Vec<u8> = Vec::new();

    for glyph in &glyphs {
        // Create padded coverage bitmap
        let mut coverage = vec![0u8; cell_w * cell_h];

        // Place glyph in cell, baseline-aligned and centered
        let glyph_y_offset =
            pad + (ascent - glyph.metrics.height as i32 - glyph.metrics.ymin as i32).max(0) as usize;
        let glyph_x_offset = pad + (cell_w - pad * 2 - glyph.metrics.width) / 2;

        for row in 0..glyph.metrics.height {
            for col in 0..glyph.metrics.width {
                let src = glyph.bitmap[row * glyph.metrics.width + col];
                let dst_x = glyph_x_offset + col;
                let dst_y = glyph_y_offset + row;
                if dst_x < cell_w && dst_y < cell_h {
                    coverage[dst_y * cell_w + dst_x] = src;
                }
            }
        }

        // Generate SDF from coverage using brute-force distance transform
        // For each pixel, find the nearest edge pixel
        let spread_sq = (spread * spread) as f32;

        for py in 0..cell_h {
            for px in 0..cell_w {
                let inside = coverage[py * cell_w + px] > 127;
                let mut min_dist_sq = spread_sq * 4.0;

                // Search in a window around the pixel
                let search = (spread.ceil() as i32) + 1;
                for sy in (py as i32 - search).max(0)..=(py as i32 + search).min(cell_h as i32 - 1)
                {
                    for sx in
                        (px as i32 - search).max(0)..=(px as i32 + search).min(cell_w as i32 - 1)
                    {
                        let other_inside =
                            coverage[sy as usize * cell_w + sx as usize] > 127;
                        if inside != other_inside {
                            let dx = (px as f32) - (sx as f32);
                            let dy = (py as f32) - (sy as f32);
                            let d = dx * dx + dy * dy;
                            if d < min_dist_sq {
                                min_dist_sq = d;
                            }
                        }
                    }
                }

                let dist = min_dist_sq.sqrt();
                // Map distance to 0-255: 128 = on edge, >128 = inside, <128 = outside
                let normalized = if inside {
                    128.0 + (dist / spread) * 127.0
                } else {
                    128.0 - (dist / spread) * 127.0
                };
                sdf_data.push(normalized.clamp(0.0, 255.0) as u8);
            }
        }
    }

    // Generate C++ header
    let total_bytes = sdf_data.len();
    let guard = var_name.to_uppercase();

    let mut out = String::new();
    out.push_str("#pragma once\n\n");
    out.push_str(&format!(
        "/// Auto-generated SDF font: {font_name} base {size_int}px (spread={spread})\n"
    ));
    out.push_str(&format!(
        "/// {num_chars} glyphs ({first_char}..{last_char}), cell {cell_w}x{cell_h}, {total_bytes} bytes\n"
    ));
    out.push_str(&format!("/// Render at any size from ~{}px to ~{}px\n\n",
        size_int / 4, size_int * 2));
    out.push_str("#include <cstdint>\n\n");
    out.push_str("#include \"lumen/gfx/sdf_font.hpp\"\n\n");
    out.push_str("namespace lumen::gfx {\n\n");

    out.push_str("// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)\n");
    out.push_str(&format!(
        "static constexpr uint8_t {guard}_DATA[] = {{\n"
    ));

    for (i, chunk) in sdf_data.chunks(20).enumerate() {
        out.push_str("    ");
        for (j, byte) in chunk.iter().enumerate() {
            out.push_str(&format!("0x{byte:02X}"));
            if i * 20 + j + 1 < total_bytes {
                out.push_str(",");
            }
        }
        out.push('\n');
    }

    out.push_str("};\n\n");

    out.push_str(&format!("inline constexpr SdfFont {var_name} = {{\n"));
    out.push_str(&format!("    .data       = {guard}_DATA,\n"));
    out.push_str(&format!("    .cell_w     = {cell_w},\n"));
    out.push_str(&format!("    .cell_h     = {cell_h},\n"));
    out.push_str(&format!("    .base_size  = {size_int},\n"));
    out.push_str(&format!("    .spread     = {spread:.0},\n"));
    out.push_str(&format!("    .first_char = {first_char},\n"));
    out.push_str(&format!("    .last_char  = {last_char},\n"));
    out.push_str("};\n\n");
    out.push_str("} // namespace lumen::gfx\n");

    fs::write(&out_path, &out).map_err(|e| format!("Failed to write {}: {e}", out_path.display()))?;

    println!(
        "  {filename}: {num_chars} glyphs, {cell_w}x{cell_h} cells, {total_bytes} bytes (SDF spread={spread})"
    );

    Ok(())
}
