# Simulation visualizations

Simple simulation visualizations.

## Building

Make sure you have the prerequisites installed for [Elementary visualizer](https://github.com/gaboroszkar/elementary_visualizer.git), and `libsvtav1enc-dev` (for AV1 video encoding).

Clone the repository and configure and build the project with the following commands.

```
git clone --recursive <repository url>
cd simulation_visualizations
cmake -S . -B build
cmake --build build
```

## Automatic reformatting

To automatically reformat the code, run the following command.
```
cmake --build build --target simulation_clangformat
```

## Licensing

This software is distributed under the GNU General Public License (GPL) version 3. You can find the full text of the license in the [LICENSE](LICENSE.txt) file.

### Other Licensing Options

Upon request, alternative licensing options may be available for this software.

