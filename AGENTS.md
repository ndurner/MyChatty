# Agent Notes

## QML Verification

- Do not treat a successful CMake build or QML cache generation as proof that QML loads correctly. Some invalid property assignments, especially when swapping Qt Quick types such as `Text` and `TextEdit`, can still fail only when `QQmlApplicationEngine` loads the component.
- After editing QML, build the target and launch the exact app binary from the active build tree. For Qt Creator debug runs in this repo, verify:

```sh
cmake --build build/Qt_6_12_0_for_macOS-Debug --target MyChatty
"./build/Qt_6_12_0_for_macOS-Debug/bin/MyChatty.app/Contents/MacOS/MyChatty"
```

- If the app exits with `QQmlApplicationEngine failed to load component`, fix the runtime QML error and repeat the launch check. The core tests are useful but do not cover QML component loading.
