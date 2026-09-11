# Dependencies

- [ctrtool 1.3.0](https://github.com/3DSGuy/Project_CTR/releases/tag/ctrtool-v1.3.0): extraction, decryption, and verification.
- [OpenSSL](https://github.com/openssl/openssl): SHA-256 on Linux. Windows uses system BCrypt instead.

The download script checks pinned archive hashes in `cmake/FetchTools.cmake`. Tool binaries are not included in the source tree.

Keep `ctrtool` in a `tools` folder beside the patcher, or use `--tools-dir`. Moving the patcher or running `cmake --install` does not move it automatically.

Release packages include the utility, its upstream source archive under `tools/sources`, and notices under `licenses`. Preserve these when redistributing the packages.

Dependencies retain their own licences. The patcher's MIT licence does not cover Minecraft or these tools.
