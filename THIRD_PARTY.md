# Dependencies

- [ctrtool 1.3.0](https://github.com/3DSGuy/Project_CTR/releases/tag/ctrtool-v1.3.0): extraction, decryption, and verification.
- [makerom 0.19.0](https://github.com/3DSGuy/Project_CTR/releases/tag/makerom-v0.19.0): one-time bootstrap CIA building.
- [OpenSSL](https://github.com/openssl/openssl): SHA-256 on Linux. Windows uses system BCrypt instead.
- [PySide6](https://www.qt.io/qt-for-python): optional Python GUI dependency, installed separately by the user.
- [IBM Plex Sans and Mono](https://github.com/IBM/plex) and [Chakra Petch](https://github.com/cadsondemak/Chakra-Petch) fonts in `assets/fonts/` are under the SIL Open Font License 1.1. See `assets/fonts/OFL.txt`.

The download script checks pinned archive hashes in `cmake/FetchTools.cmake`. Tool binaries are not included in the source tree.

Keep `ctrtool` and `makerom` in a `tools` folder beside the patcher, or use `--tools-dir`. Moving the patcher or running `cmake --install` does not move them automatically.

Release packages include both utilities, their upstream source archives under `tools/sources`, and notices under `licenses`. Preserve these when redistributing the packages.

Dependencies retain their own licences. The patcher's MIT licence does not cover Minecraft or these tools.
