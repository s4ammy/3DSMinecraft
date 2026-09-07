# Dependencies

- [ctrtool 1.3.0](https://github.com/3DSGuy/Project_CTR/releases/tag/ctrtool-v1.3.0): extraction, decryption, and verification.
- [makerom 0.19.0](https://github.com/3DSGuy/Project_CTR/releases/tag/makerom-v0.19.0): CIA rebuilding.
- [OpenSSL](https://github.com/openssl/openssl): SHA-256 on Linux. Windows uses system BCrypt instead.

The download script checks pinned archive hashes in `cmake/FetchTools.cmake`. Tool binaries are not included in the source tree.

Keep `ctrtool` and `makerom` in a `tools` folder beside the patcher, or use `--tools-dir`. Moving the patcher or running `cmake --install` does not move these tools automatically.

Release packages include the utilities, their upstream source archives under `tools/sources`, and notices under `licenses`. Makerom's bundled BLZ code uses GPLv3 or later; the complete tagged source and GPL text accompany the binary. Preserve these when redistributing the packages.

Dependencies retain their own licences. The patcher's MIT licence does not cover Minecraft or these tools.
