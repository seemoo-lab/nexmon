# External inputs: exact sources and necessity

Binary dependencies are not omitted from the reproduction instructions. They are obtained separately from their provider or the owner's matching device, then checked locally. They are not copied into this contribution.

| Input | Required for | Source |
|---|---|---|
| Exact stock `hans.trx`,527400bytes | Building the validated image and restoring stock | Owner's matching iPhone8,4, iOS15.7/19H12: `/usr/share/firmware/wifi/C-43452__s-A3/hans.trx`. No verified standalone official download URL is established. Copy the existing file without changing it; never substitute nearby-chip firmware. |
| Installed Apple `wifiFirmwareLoader` | Loading/restoring firmware on the matching phone | Existing `/usr/libexec/wifiFirmwareLoader` from that installed iOS build; do not replace it with a reference binary. |
| Dopamine3.0.11 and its `libjailbreak.dylib` | Exact supported jailbreak primitives used by the loader transaction | [Official release](https://github.com/opa334/Dopamine/releases/tag/3.0.11), [direct official IPA](https://github.com/opa334/Dopamine/releases/download/3.0.11/Dopamine.ipa). The downloaded IPA's bundled library was checked against the actual installed library; hashes below. |
| Zig0.13.0 | Exact Thumb firmware-source compilation | [Official downloads](https://ziglang.org/download/), [0.13.0 release](https://ziglang.org/download/0.13.0/release-notes.html). Select0.13.0 for the host platform; a newer compiler is not an exact-build substitute. |
| Capstone5.0.9, Keystone-engine0.9.2 | Narrow instruction validation/branch relocation in the host builder | [Capstone package](https://pypi.org/project/capstone/5.0.9/), [Keystone package](https://pypi.org/project/keystone-engine/0.9.2/), pinned in `requirements.txt`. |
| iOS SDK and native compiler/signing tools | Compiling the phone-side native utilities from source | [Apple Xcode resources](https://developer.apple.com/xcode/resources/), [Apple downloads](https://developer.apple.com/download/all/), [Procursus](https://github.com/ProcursusTeam/Procursus) for matching jailbreak tool packages. The original build used a locally held SDK labelled15.6 and Clang16; that label alone does not establish a separately downloadable official SDK15.6 release. Obtain a lawful compatible SDK; native rebuilt helpers require their ABI/self-tests rather than an assumed byte-identical binary. |
| Exact partial overlaid ROM and reference kernelcache | Repeating the reverse-engineering/emulation investigation | Privately obtained from the matching owner-controlled device/firmware. Metadata records range and SHA256. Neither is read by the fixed-profile source builder or required as a runtime file for the demonstrated capture route. No verified public exact ROM URL is established. |

## Verification pins

- Stock image SHA256: `280bc2d7ad25f2c789ce332c37da9d687693fbda156830803fe1516e8d938244`.
- Original loader SHA256: `a391c0e1c13e161434d4063ade9b9f95669d81cc81382a435adc17519f0a3227`.
- Official Dopamine3.0.11 IPA SHA256: `fbd5b9086634cac0b6c7e5ba95c43d8c40ecafd814f8e620beb06f355480b973`.
- Its embedded `basebin.tar` SHA256: `cb6e61f51d06c70afe81ba716b7d46157e210e5c0d052823b0fe6496b416e8df`.
- Bundled and installed `libjailbreak.dylib` SHA256: `ea76a9d9b6af0a50c0aa2ace1f770ad5cfcfff89c4555b2c60dc4b7d1b4427be`.
- Rebuilt candidate SHA256: `0245b815b9ff0a5d7e7a3b4f42b491338d2dbeefd8f5bf66cf8a86a83fb00f1f`.

An already compatible installed iOS is a prerequisite; this contribution does not promise that Apple currently permits installing/downgrading to that build. Copying a reference loader/library over a mismatching new system is not a supported workaround. Stop on a pin mismatch. ROM/kernel inputs are necessary only for repeating the investigation, not for reproducing the fixed exact-target source build. No private network constants or captured authentication material are necessary dependencies.
