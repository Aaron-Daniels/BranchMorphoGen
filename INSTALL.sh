#!/bin/bash

# Exit on error
set -e

# Determine if user passed a custom install path
if [ "$1" != "" ]; then
  INSTALL_PREFIX="$1"
else
  # Check if running as root
  if [ "$EUID" -eq 0 ]; then
    INSTALL_PREFIX="/usr/local"
  else
    INSTALL_PREFIX="$HOME/.local"
  fi
fi

echo "Installing to: $INSTALL_PREFIX"

# Clean and create build directory
rm -rf build
mkdir -p build
cd build

# Configure the project
cmake .. -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"

# Build
make -j1

# Install
make install

# Suggest PATH update if needed (only for user local installs)
if [ "$INSTALL_PREFIX" != "/usr/local" ] && [[ ":$PATH:" != *":$INSTALL_PREFIX/bin:"* ]]; then
  echo ""
  echo "######  ATTENTION: $INSTALL_PREFIX/bin is not in your PATH."
  echo "Add the following line to your shell config file (for example ~/.bashrc or ~/.zshrc):"
  echo "    export PATH=\"$INSTALL_PREFIX/bin:\$PATH\""
  echo "Then run: source ~/.bashrc (or source ~/.zshrc)"
fi

# Print a branched neuron
cat << 'EOF'
      \  |  /
       \ | /
        \|/
  ------ O ------
        /|\
       / | \
      /  |  \
EOF
echo ""
echo "###### Installation complete!"
echo "Binary installed to: $INSTALL_PREFIX/bin/"
echo "Headers (if any) installed to: $INSTALL_PREFIX/include/"

echo ""
echo "======================================================================"
echo "   BranchMorphoGen"
echo "   Branching Morphology Generator"
echo "   Developed by Sabyasachi Sutradhar, sabyasachi.sutradhar@yale.edu"
echo "   © Sabyasachi Sutradhar, Yale University"
echo "======================================================================"
echo ""

