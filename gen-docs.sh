#!/bin/bash
set -e

# ====================================================
# This script requires the doxygen and graphviz tools.
# Install them with following command if required:
#     sudo apt-get install -y doxygen graphviz
# ====================================================

# generate the documentation website
doxygen Doxyfile
cp -r include html/
zip -r vebtree-docs-html.zip html
rm -rf html latex
