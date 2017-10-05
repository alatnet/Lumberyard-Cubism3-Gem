Place develop and release library folders from Lib2DCubismCore/lib here
Place "Live2DCubismCore.h" into "include".

For Native components place "Live2DCubismFramework.h" and "Live2DCubismFrameworkINTERNAL.h" into "include".  
Place Framework source files into "Source\Framework".

Folder Structure:
```
3rdParty/
├── develop
│   ├── android
│   │   ├── armeabi-v7a
│   │   │   └── libLive2DCubismCore.a
│   ├── ios
│   │   └── libLive2DCubismCore.a
│   ├── linux
│   │   └── x86_64
│   │       └── libLive2DCubismCore.a
│   ├── macos
│   │   └── libLive2DCubismCore.a
│   └── windows
│       └── x86_64
│           └── Live2DCubismCore.lib
├── include
│   ├── Live2DCubismCore.h
│   ├── Live2DCubismFramework.h (optional)
│   └── Live2DCubismFrameworkINTERNAL.h (optional)
├── release
│   ├── android
│   │   ├── armeabi-v7a
│   │   │   └── libLive2DCubismCore.a
│   ├── ios
│   │   └── libLive2DCubismCore.a
│   ├── linux
│   │   └── x86_64
│   │       └── libLive2DCubismCore.a
│   ├── macos
│   │   └── libLive2DCubismCore.a
│   └── windows
│       └── x86_64
│           └── Live2DCubismCore.lib
└── source (optional)
    └── Framework
        ├── Animation.c
        ├── AnimationSegmentEvaluationFunction.c
        ├── AnimationState.c
        ├── FloatBlendFunction.c
        ├── Json.c
        ├── Local.h
        ├── ModelExtensions.c
        ├── MotionJson.c
        ├── Physics.c
        ├── PhysicsJson.c
        ├── PhysicsMath.c
        └── String.c
```


