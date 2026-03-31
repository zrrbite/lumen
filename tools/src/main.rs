mod font;
mod image;

use clap::{Parser, Subcommand};

#[derive(Parser)]
#[command(name = "lumen-tools", about = "Asset pipeline for Lumen embedded GUI")]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    /// Convert a TTF/OTF font to a C++ bitmap font header
    Font {
        /// Path to the TTF/OTF font file
        input: String,

        /// Comma-separated list of pixel sizes to generate
        #[arg(short, long, default_value = "14")]
        sizes: String,

        /// Output directory for generated .hpp files
        #[arg(short, long, default_value = ".")]
        output: String,

        /// Variable name prefix (default: derived from font filename)
        #[arg(short, long)]
        name: Option<String>,

        /// First ASCII character to include
        #[arg(long, default_value_t = 32)]
        first_char: u8,

        /// Last ASCII character to include
        #[arg(long, default_value_t = 126)]
        last_char: u8,
    },

    /// Convert a PNG image to a C++ pixel data header
    Image {
        /// Path to the PNG image
        input: String,

        /// Output .hpp file path
        #[arg(short, long)]
        output: Option<String>,

        /// Pixel format: rgb565 or argb8888
        #[arg(short, long, default_value = "rgb565")]
        format: String,
    },
}

fn main() {
    let cli = Cli::parse();

    match cli.command {
        Commands::Font {
            input,
            sizes,
            output,
            name,
            first_char,
            last_char,
        } => {
            let sizes: Vec<f32> = sizes
                .split(',')
                .filter_map(|s| s.trim().parse().ok())
                .collect();

            if sizes.is_empty() {
                eprintln!("Error: no valid sizes specified");
                std::process::exit(1);
            }

            let name_prefix = name.unwrap_or_else(|| {
                std::path::Path::new(&input)
                    .file_stem()
                    .and_then(|s| s.to_str())
                    .unwrap_or("font")
                    .to_lowercase()
                    .replace(['-', ' ', '.'], "_")
            });

            if let Err(e) =
                font::convert(&input, &sizes, &output, &name_prefix, first_char, last_char)
            {
                eprintln!("Error: {e}");
                std::process::exit(1);
            }
        }
        Commands::Image {
            input,
            output,
            format,
        } => {
            if let Err(e) = image::convert(&input, output.as_deref(), &format) {
                eprintln!("Error: {e}");
                std::process::exit(1);
            }
        }
    }
}
