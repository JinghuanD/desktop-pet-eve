# Eve Desktop Pet

Eve is a Qt desktop-pet application with selectable outfits, animated expressions, reading mode, health tracking, and utility panels.

## Build

Open `Lynn.pro` in Qt Creator, select a Qt 5.14.2 MinGW kit, and build the project. The build target is `Eve`.

## Runtime notes

The health lookup feature uses the USDA FoodData Central API. Configure the `USDA_API_KEY` environment variable locally before using it.
