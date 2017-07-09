# Lumberyard-Cubism3-Gem
An Amazon Lumberyard Gem that adds in Live2D Cubism3 functionality to LyShine.

## Requirements
To compile the gem, it requires that you have downloaded Cubism3 SDK from [live2d](https://live2d.github.io/#native).  
Place the SDK\'s develop and release library files in "3rdParty" folder and the SDK\'s include files in "Code\\Source".  

## How to have asset listing
In order to have the editor list Cubism3 assets you will need to add the following to your "Amazon/\<version\>/dev/AssetProcessorPlatformConfig.ini":  
```Text
; Copy Moc Files
[RC Cubism3]
glob=*.moc3
params=copy
productAssetType={7DB33C8B-8498-404C-A301-B0269AE60388}

; Copy Model3 Files
[RC Cubism3_Model3]
glob=*.model3.json
params=copy
productAssetType={A679F1C0-60A1-48FB-8107-A68195D76CF2}
```
