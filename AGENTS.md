# Agent Notes

## QML Verification

- Do not treat a successful CMake build or QML cache generation as proof that QML loads correctly. Some invalid property assignments, especially when swapping Qt Quick types such as `Text` and `TextEdit`, can still fail only when `QQmlApplicationEngine` loads the component.
- After editing QML, build the target and launch the exact app binary from the active build tree. For Qt Creator debug runs in this repo, verify:

```sh
cmake --build build/Qt_6_12_0_for_macOS-Debug --target MyChatty
"./build/Qt_6_12_0_for_macOS-Debug/bin/MyChatty.app/Contents/MacOS/MyChatty"
```

- If the app exits with `QQmlApplicationEngine failed to load component`, fix the runtime QML error and repeat the launch check. The core tests are useful but do not cover QML component loading.

## API Cost Safety

- Unit and core tests must remain offline. Test request payload construction and response parsing with local objects or fixtures; never send requests to vendor APIs from `ctest`.
- Building, running `mychatty_core_tests`, and launching the app normally must not consume API credits, even when API keys are configured.
- `--ui-state=e2e-openrouter` is an explicit paid end-to-end mode that sends model requests. Do not use it for routine build or QML verification.
- Use `mychatty-cli --dry-run` when verifying request JSON. Running the CLI without `--dry-run` may make a paid request when there is no cache hit.

## Model and Provider Additions

- When adding or changing a provider or model, verify its current reasoning-effort capabilities from the vendor's documentation or model metadata. Do not assume every model exposed by one provider accepts the same effort values or request shape.
- Update the model-specific selector options and payload mapping together. Models without discrete effort control should omit the effort parameter and expose only `Default`; unsupported levels should not appear in the UI.
- Extend the offline model-by-effort payload matrix in `tests/test_core.cpp` for every new catalog entry. Keep this as serialization coverage only; do not add paid vendor calls to the test suite.
