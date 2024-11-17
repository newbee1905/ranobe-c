#!/bin/dash

# Paths
REGEX_DIR="regex"
COMPILED_REGEX_DIR="compiled_regex"
COMPILE_REGEX_SRC="scripts/compile_regex.c"
COMPILE_REGEX_BIN="./scripts/compile_regex"

# Ensure the compiled regex directory exists
mkdir -p "$COMPILED_REGEX_DIR"

# Check if compile_regex utility exists, build it if necessary
if ! [ -x "$COMPILE_REGEX_BIN" ]; then
  if [ -f "$COMPILE_REGEX_SRC" ]; then
    echo "Building compile_regex utility..."
    gcc -o "$COMPILE_REGEX_BIN" "$COMPILE_REGEX_SRC" -lpcre -flto -Ofast
    if [ $? -ne 0 ]; then
      echo "Error: Failed to build compile_regex utility."
      exit 1
    fi
    echo "compile_regex utility built successfully."
  else
    echo "Error: compile_regex.c source file not found."
    exit 1
  fi
fi

find "$REGEX_DIR" -type f -name "*.regex" | while read -r regex_file; do
	relative_path="${regex_file#$REGEX_DIR/}"
	output_dir="$COMPILED_REGEX_DIR/$(dirname "$relative_path")"
	mkdir -p "$output_dir"

	compiled_file="$output_dir/$(basename "$regex_file" .regex).pcre"

  echo "Compiling regex: $regex_file -> $compiled_file"
  "$COMPILE_REGEX_BIN" "$regex_file" "$compiled_file"

  # Check if compilation was successful
  if [ $? -ne 0 ]; then
    echo "Error: Failed to compile $regex_file"
    exit 1
  fi
done

echo "All regex patterns compiled successfully into $COMPILED_REGEX_DIR"

