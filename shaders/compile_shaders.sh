find . -type f \( \
    -name "*.vert" -o \
    -name "*.frag" -o \
    -name "*.comp" -o \
    -name "*.geom" -o \
    -name "*.tesc" -o \
    -name "*.tese" -o \
    -name "*.mesh" -o \
    -name "*.task" -o \
    -name "*.rgen" -o \
    -name "*.rchit" -o \
    -name "*.rahit" -o \
    -name "*.rmiss" -o \
    -name "*.rcall" -o \
    -name "*.glsl" \
\) | while IFS= read -r shader; do
    output="${shader}.spv"
    echo "Compiling: $shader -> $output"

    if glslc "$shader" -o "$output"; then
        echo "Successfully compiled"
    else
        echo "Compilation failed my man"
    fi
done