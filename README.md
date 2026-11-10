# Eve Desktop Pet

Eve is a lightweight Qt desktop companion with animated character portraits, a reading companion, health tracking, and small desktop utilities.

## Features

- **Three complete outfits** — choose the Mid-Autumn, Bunny Pajama, or Cat Pajama portrait in the Dress panel.
- **Animated expressions** — Eve blinks automatically every two seconds. Left-click Eve to cycle through the available happy, surprised, and sleepy expressions.
- **Reading mode** — switches Eve to a reading pose and shows a companion card above the pet. It tracks the current Microsoft Edge PDF or Microsoft Word document title, reading time, and, where available, page progress.
- **Health manager** — record meals, water, and steps by date; add or edit calories in either `kcal` or `cal`; browse earlier records with the date picker.
- **USDA FoodData Central lookup** — search an English food name to obtain a calorie estimate from the USDA database.
- **Timer and game panels** — launch a focus timer or view foreground-game activity from the menu.
- **Tray controls** — hide Eve to the system tray, restore it by clicking the tray icon, or quit from the tray menu.
- **Settings** — configure startup and other desktop-pet options.

## Using Eve

1. Run `Eve.exe`.
2. Left-click the pet to open the paw menu.
3. Select **Dress** to change the full portrait, or select **Reading** to enter or leave reading mode.
4. In reading mode, open a PDF in Microsoft Edge or a document in Microsoft Word. The card will stay with Eve and preserve elapsed time while you switch to other non-document windows.
5. Select **Health** to log a meal. Enter an English food name and a serving amount, then choose **USDA Lookup Calories**. You can also enter calories manually and edit a selected record later.

## USDA setup

The USDA lookup requires a free FoodData Central API key. Store it locally as an environment variable; do not commit the key to this repository.

```powershell
[Environment]::SetEnvironmentVariable('USDA_API_KEY', 'your-api-key', 'User')
```

Restart Eve after adding or changing the key.
