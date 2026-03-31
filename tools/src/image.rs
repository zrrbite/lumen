use std::fs;
use std::path::Path;

pub fn convert(input: &str, output: Option<&str>, format: &str) -> Result<(), String> {
    let file = fs::File::open(input).map_err(|e| format!("Failed to open {input}: {e}"))?;
    let decoder = png::Decoder::new(file);
    let mut reader = decoder
        .read_info()
        .map_err(|e| format!("Failed to decode PNG: {e}"))?;

    let mut buf = vec![0u8; reader.output_buffer_size()];
    let info = reader
        .next_frame(&mut buf)
        .map_err(|e| format!("Failed to read PNG frame: {e}"))?;

    let width = info.width as usize;
    let height = info.height as usize;

    let pixels_rgb: Vec<(u8, u8, u8)> = match info.color_type {
        png::ColorType::Rgb => buf[..width * height * 3]
            .chunks_exact(3)
            .map(|c| (c[0], c[1], c[2]))
            .collect(),
        png::ColorType::Rgba => buf[..width * height * 4]
            .chunks_exact(4)
            .map(|c| (c[0], c[1], c[2]))
            .collect(),
        png::ColorType::Grayscale => buf[..width * height]
            .iter()
            .map(|&g| (g, g, g))
            .collect(),
        _ => return Err(format!("Unsupported color type: {:?}", info.color_type)),
    };

    let out_path = match output {
        Some(p) => p.to_string(),
        None => {
            let stem = Path::new(input)
                .file_stem()
                .and_then(|s| s.to_str())
                .unwrap_or("image");
            format!("{stem}.hpp")
        }
    };

    let var_name = Path::new(&out_path)
        .file_stem()
        .and_then(|s| s.to_str())
        .unwrap_or("image")
        .to_lowercase()
        .replace(['-', ' ', '.'], "_");

    let header = match format {
        "rgb565" => generate_rgb565(&pixels_rgb, width, height, &var_name),
        "argb8888" => generate_argb8888(&pixels_rgb, width, height, &var_name),
        _ => return Err(format!("Unknown pixel format: {format}")),
    };

    fs::write(&out_path, header).map_err(|e| format!("Failed to write {out_path}: {e}"))?;

    let bytes = match format {
        "rgb565" => width * height * 2,
        _ => width * height * 4,
    };
    println!(
        "  {out_path}: {width}x{height} {format}, {} bytes",
        bytes
    );

    Ok(())
}

fn generate_rgb565(pixels: &[(u8, u8, u8)], width: usize, height: usize, var_name: &str) -> String {
    let guard = var_name.to_uppercase();
    let total = pixels.len();

    let mut out = String::new();
    out.push_str("#pragma once\n\n");
    out.push_str(&format!(
        "/// Auto-generated image: {width}x{height} RGB565\n"
    ));
    out.push_str(&format!("/// {total} pixels, {} bytes\n\n", total * 2));
    out.push_str("#include <cstdint>\n\n");
    out.push_str("namespace lumen::assets {\n\n");
    out.push_str(&format!("static constexpr uint16_t {guard}_W = {width};\n"));
    out.push_str(&format!("static constexpr uint16_t {guard}_H = {height};\n\n"));

    out.push_str(&format!(
        "// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)\n"
    ));
    out.push_str(&format!(
        "static constexpr uint16_t {guard}_DATA[] = {{\n"
    ));

    let rgb565: Vec<u16> = pixels
        .iter()
        .map(|&(r, g, b)| {
            let r5 = (r as u16 >> 3) << 11;
            let g6 = (g as u16 >> 2) << 5;
            let b5 = b as u16 >> 3;
            r5 | g6 | b5
        })
        .collect();

    for (i, chunk) in rgb565.chunks(8).enumerate() {
        out.push_str("    ");
        for (j, &px) in chunk.iter().enumerate() {
            out.push_str(&format!("0x{px:04X}"));
            if i * 8 + j + 1 < total {
                out.push_str(", ");
            }
        }
        out.push('\n');
    }

    out.push_str("};\n\n");
    out.push_str("} // namespace lumen::assets\n");

    out
}

fn generate_argb8888(
    pixels: &[(u8, u8, u8)],
    width: usize,
    height: usize,
    var_name: &str,
) -> String {
    let guard = var_name.to_uppercase();
    let total = pixels.len();

    let mut out = String::new();
    out.push_str("#pragma once\n\n");
    out.push_str(&format!(
        "/// Auto-generated image: {width}x{height} ARGB8888\n"
    ));
    out.push_str(&format!("/// {total} pixels, {} bytes\n\n", total * 4));
    out.push_str("#include <cstdint>\n\n");
    out.push_str("namespace lumen::assets {\n\n");
    out.push_str(&format!("static constexpr uint16_t {guard}_W = {width};\n"));
    out.push_str(&format!("static constexpr uint16_t {guard}_H = {height};\n\n"));

    out.push_str(&format!(
        "// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)\n"
    ));
    out.push_str(&format!(
        "static constexpr uint32_t {guard}_DATA[] = {{\n"
    ));

    for (i, chunk) in pixels.chunks(6).enumerate() {
        out.push_str("    ");
        for (j, &(r, g, b)) in chunk.iter().enumerate() {
            let argb = 0xFF000000u32 | ((r as u32) << 16) | ((g as u32) << 8) | b as u32;
            out.push_str(&format!("0x{argb:08X}"));
            if i * 6 + j + 1 < total {
                out.push_str(", ");
            }
        }
        out.push('\n');
    }

    out.push_str("};\n\n");
    out.push_str("} // namespace lumen::assets\n");

    out
}
