---
name: docs
description: Generate the Doxygen documentation website for veb-in-c and report where the output zip landed. Use when the user wants to refresh docs after public-API or comment changes.
---

# /docs

Regenerates the Doxygen HTML site from the repo's `Doxyfile` and packages it as a zip.

## Steps

1. From the repo root, run `./gen-docs.sh`.
2. Expected output: `vebtree-docs-html.zip` in the repo root.

## Report

- Whether Doxygen ran cleanly. Surface any new warnings — but the existing code gates the implementation behind `#ifndef DOXYGEN_SKIP`, so warnings about the gated block being "skipped" are expected noise, not real problems.
- The path to the generated zip.
- To preview: unzip it and open `vebtree-docs-html/html/index.html` in a browser.

## Prerequisites

If `doxygen`, `graphviz`, or `zip` is missing, point the user at the README's install line:

```sh
sudo apt-get install -y doxygen graphviz zip unzip
```
