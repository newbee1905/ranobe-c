{
	description = "Flake for building ranobe binary and compiling regex patterns";
	
	inputs = {
		nixpkgs.url = "nixpkgs/nixos-unstable";
		systems.url = "github:nix-systems/default";
	};

	outputs = { self, nixpkgs, systems }: let
		forAllSystems = nixpkgs.lib.genAttrs (import systems);
		pkgsFor = system: import nixpkgs { 
			inherit system;
			config.allowUnfree = true;
		};
	in {
		packages = forAllSystems (system: let
			pkgs = pkgsFor system;
		in rec {
			regexCompiler = pkgs.stdenv.mkDerivation {
				pname = "compile-regex";
				version = "0.1";
				src = ./scripts;
				
				nativeBuildInputs = [ pkgs.clang ];
				buildInputs = [ pkgs.pcre ];
				
				CFLAGS = [
					"-O3"
					"-flto"
					"-march=native"
					"-Wall"
					"-Wextra"
				];
				
				buildPhase = ''
					${pkgs.clang}/bin/clang -o compile_regex compile_regex.c -lpcre $CFLAGS
				'';
				
				installPhase = ''
					mkdir -p $out/bin
					install -Dm755 compile_regex $out/bin/compile_regex
				'';
			};

			ranobe = pkgs.stdenv.mkDerivation {
				pname = "ranobe";
				version = "0.0.1";
				
				srcs = [ ./src ./include ./regex ];
				sourceRoot = "src";
				
				nativeBuildInputs = [ 
					pkgs.clang
					regexCompiler
				];
				buildInputs = [ pkgs.curl pkgs.pcre ];
				
				CFLAGS = [
					"-O3"
					"-flto"
					"-march=native"
					"-Wall"
					"-Wextra"
				];
				
				buildPhase = ''
					# Create the compiled_regex directory
					mkdir -p compiled_regex
					
					# Find and compile all regex patterns
					find ../regex -type f -name "*.regex" | while read -r regex_file; do
						relative_path=''${regex_file#../regex/}
						output_dir=compiled_regex/$(dirname "$relative_path")
						mkdir -p "$output_dir"
						compiled_file=$output_dir/$(basename "$regex_file" .regex).pcre
						
						if ! compile_regex "$regex_file" "$compiled_file"; then
							echo "Error compiling regex pattern: $regex_file"
							exit 1
						fi
					done
					
					${pkgs.clang}/bin/clang -I../include $CFLAGS -o ranobe ./*.c -lcurl -lpcre
				'';
				
				installPhase = ''
					mkdir -p $out/bin
					mkdir -p $out/compiled_regex
					
					install -Dm755 ranobe $out/bin/ranobe
					cp -r compiled_regex/* $out/compiled_regex/
				'';
				
				meta = with pkgs.lib; {
					description = "Ranobe binary with compiled regex patterns";
					platforms = platforms.linux;
					maintainers = [ newbee1905 ];
					license = licenses.mit;
				};
			};

			default = ranobe;
		});
	};
}
