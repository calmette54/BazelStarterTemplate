# Changelog

## [3.2.0](https://github.com/calmette54/BazelStarterTemplate/compare/v3.1.0...v3.2.0) (2026-10-28)


### Features

* **bazel:** Upgrade Bazel to 9.3.1 ([#144](https://github.com/calmette54/BazelStarterTemplate/issues/144)) ([d82203f](https://github.com/calmette54/BazelStarterTemplate/commit/d82203f5ba5887954795dfc4a864a5893a29fe93))


### Bug Fixes

* **devenv:** declare git-hooks input for devenv 2.x ([#142](https://github.com/calmette54/BazelStarterTemplate/issues/142)) ([a53b253](https://github.com/calmette54/BazelStarterTemplate/commit/a53b253fab37659a409df6d03506e6324845a9df))

## [3.1.0](https://github.com/calmette54/BazelStarterTemplate/compare/v3.0.0...v3.1.0) (2026-10-22)


### Features

* **examples:** Add eigen_go cgo example ([#136](https://github.com/calmette54/BazelStarterTemplate/issues/136)) ([2903a34](https://github.com/calmette54/BazelStarterTemplate/commit/2903a342be0def119c7144d01285e4aed0e9939f))
* **examples:** Add eigen_pybind example as a separate module ([#135](https://github.com/calmette54/BazelStarterTemplate/issues/135)) ([d07c920](https://github.com/calmette54/BazelStarterTemplate/commit/d07c920b5a682f7aa10725ffe4b0e1d1748f5eb0))
* **examples:** Add gazebo_tutorial example (build only) ([#140](https://github.com/calmette54/BazelStarterTemplate/issues/140)) ([9c87bff](https://github.com/calmette54/BazelStarterTemplate/commit/9c87bff7e5a16c0912c9ef1eee90a069e57f6a61))
* **examples:** Add lidar_3d Gazebo example ([#139](https://github.com/calmette54/BazelStarterTemplate/issues/139)) ([7b4fba6](https://github.com/calmette54/BazelStarterTemplate/commit/7b4fba61c664b9f0a79230730fdc2210b680b2ff))
* **examples:** Add open3d_prebuilt example ([#138](https://github.com/calmette54/BazelStarterTemplate/issues/138)) ([eca0ef7](https://github.com/calmette54/BazelStarterTemplate/commit/eca0ef70a80951e406c6a323ef7edafc5a6ef64f))
* **examples:** Add python_ai example with Gazelle-generated BUILD files ([#137](https://github.com/calmette54/BazelStarterTemplate/issues/137)) ([c575c0f](https://github.com/calmette54/BazelStarterTemplate/commit/c575c0fdb30f07fc3b64dadef275eb6327b06c32))
* **plugin:** Add the bazel-starter Claude Code plugin ([#141](https://github.com/calmette54/BazelStarterTemplate/issues/141)) ([7d47efa](https://github.com/calmette54/BazelStarterTemplate/commit/7d47efaf06b5c8f0772c86ccb6aa788d1213d31b))


### Bug Fixes

* **core:** Repair release stamping, aarch64 toolchain and template defaults ([#128](https://github.com/calmette54/BazelStarterTemplate/issues/128)) ([1e51959](https://github.com/calmette54/BazelStarterTemplate/commit/1e519590f164263245e3ef33a9c2bba338ccf023))
* **pre-commit:** formatter hook ([#127](https://github.com/calmette54/BazelStarterTemplate/issues/127)) ([2e55c36](https://github.com/calmette54/BazelStarterTemplate/commit/2e55c362236e2dd45d1eb1c9923aa5caee2eda97))
* **toolchain:** compile with C++23 as configured ([#131](https://github.com/calmette54/BazelStarterTemplate/issues/131)) ([cb83bdd](https://github.com/calmette54/BazelStarterTemplate/commit/cb83bddaa36cadcddec7896d93303cd312e14d5c))

## [3.0.0](https://github.com/alemoreno991/BazelStarterTemplate/compare/v2.0.0...v3.0.0) (2025-04-28)


### ⚠ BREAKING CHANGES

* **nixos-compatibility:** Closes #31

### Features

* **cross-compile:** cross-compile from x86_64 to aarch64 ([#50](https://github.com/alemoreno991/BazelStarterTemplate/issues/50)) ([6084954](https://github.com/alemoreno991/BazelStarterTemplate/commit/608495419391c722bc43a44579c1bdab748c0122)), closes [#11](https://github.com/alemoreno991/BazelStarterTemplate/issues/11)
* fake ([7604e6e](https://github.com/alemoreno991/BazelStarterTemplate/commit/7604e6eb97e726eacac528b7cf4a604cf1462b2e))
* fake ([0d22bb6](https://github.com/alemoreno991/BazelStarterTemplate/commit/0d22bb618686e9cd76ba2e83bf295a0ddd15fccd))
* fake ([1a073d9](https://github.com/alemoreno991/BazelStarterTemplate/commit/1a073d92ddbfd504e4f81459fb2f5321eb32566d))
* fake ([b7b3041](https://github.com/alemoreno991/BazelStarterTemplate/commit/b7b3041fa40239bef9df23336252b33f88715b5a))
* mimic a meaningful change to the code ([8a3b164](https://github.com/alemoreno991/BazelStarterTemplate/commit/8a3b16469058068ea4275c1adbd8d20ad83fa895))
* **semVer:** semantic versioning of artifacts ([#53](https://github.com/alemoreno991/BazelStarterTemplate/issues/53)) ([255e9e3](https://github.com/alemoreno991/BazelStarterTemplate/commit/255e9e3c45f0768ff91ca10c1cc4b9266dc77519)), closes [#38](https://github.com/alemoreno991/BazelStarterTemplate/issues/38)


### Bug Fixes

* **credential-helper:** fix unreliable behavior ([#52](https://github.com/alemoreno991/BazelStarterTemplate/issues/52)) ([80c6f3d](https://github.com/alemoreno991/BazelStarterTemplate/commit/80c6f3d3f0f8ac613a9e31a95424ab887461842f))
* **credential-helper:** install credential-helper ([#46](https://github.com/alemoreno991/BazelStarterTemplate/issues/46)) ([1f9e886](https://github.com/alemoreno991/BazelStarterTemplate/commit/1f9e88677e42c90518b96f9e2cff12abdc158d2c)), closes [#45](https://github.com/alemoreno991/BazelStarterTemplate/issues/45)
* Documentation quick fix ([58c24ab](https://github.com/alemoreno991/BazelStarterTemplate/commit/58c24abd3613ac6c2ffaea5ced92b8d84c70499c))
* Modify the release-please configuration ([29e696c](https://github.com/alemoreno991/BazelStarterTemplate/commit/29e696c51021d1b31f80a5d12cbfe52e63a017d3))
* **pre-commit:** pre-commit hook for commitizen ([#25](https://github.com/alemoreno991/BazelStarterTemplate/issues/25)) ([885ddf4](https://github.com/alemoreno991/BazelStarterTemplate/commit/885ddf4f8eb636c42e9d327574b5db0bbeebb569))
* release-please write access to issues ([3de0aa3](https://github.com/alemoreno991/BazelStarterTemplate/commit/3de0aa3a5814c65779db416b8d9a42637b03606c))
* **release-please:** fix release please config ([577b63c](https://github.com/alemoreno991/BazelStarterTemplate/commit/577b63c9611f3a9874cc62523f6540ce25e28fd2))


### Performance Improvements

* **ci:** avoid copying tarball unnecessarily ([#112](https://github.com/alemoreno991/BazelStarterTemplate/issues/112)) ([a7bf536](https://github.com/alemoreno991/BazelStarterTemplate/commit/a7bf53684663f2ec48faf13a1446bf83f3fcdda2))


### Build System

* **nixos-compatibility:** build system compatible with nixos ([#32](https://github.com/alemoreno991/BazelStarterTemplate/issues/32)) ([7b2718c](https://github.com/alemoreno991/BazelStarterTemplate/commit/7b2718cbbb39e8647a597e85ddb7ffcbfb4d5ad1))

## [2.0.0](https://github.com/alemoreno991/BazelStarterTemplate/compare/v1.3.0...v2.0.0) (2025-04-28)


### ⚠ BREAKING CHANGES

* **nixos-compatibility:** Closes #31

### Features

* **cross-compile:** cross-compile from x86_64 to aarch64 ([#50](https://github.com/alemoreno991/BazelStarterTemplate/issues/50)) ([6084954](https://github.com/alemoreno991/BazelStarterTemplate/commit/608495419391c722bc43a44579c1bdab748c0122)), closes [#11](https://github.com/alemoreno991/BazelStarterTemplate/issues/11)
* **semVer:** semantic versioning of artifacts ([#53](https://github.com/alemoreno991/BazelStarterTemplate/issues/53)) ([255e9e3](https://github.com/alemoreno991/BazelStarterTemplate/commit/255e9e3c45f0768ff91ca10c1cc4b9266dc77519)), closes [#38](https://github.com/alemoreno991/BazelStarterTemplate/issues/38)


### Bug Fixes

* **credential-helper:** fix unreliable behavior ([#52](https://github.com/alemoreno991/BazelStarterTemplate/issues/52)) ([80c6f3d](https://github.com/alemoreno991/BazelStarterTemplate/commit/80c6f3d3f0f8ac613a9e31a95424ab887461842f))
* **credential-helper:** install credential-helper ([#46](https://github.com/alemoreno991/BazelStarterTemplate/issues/46)) ([1f9e886](https://github.com/alemoreno991/BazelStarterTemplate/commit/1f9e88677e42c90518b96f9e2cff12abdc158d2c)), closes [#45](https://github.com/alemoreno991/BazelStarterTemplate/issues/45)
* **pre-commit:** pre-commit hook for commitizen ([#25](https://github.com/alemoreno991/BazelStarterTemplate/issues/25)) ([885ddf4](https://github.com/alemoreno991/BazelStarterTemplate/commit/885ddf4f8eb636c42e9d327574b5db0bbeebb569))


### Performance Improvements

* **ci:** avoid copying tarball unnecessarily ([#112](https://github.com/alemoreno991/BazelStarterTemplate/issues/112)) ([a7bf536](https://github.com/alemoreno991/BazelStarterTemplate/commit/a7bf53684663f2ec48faf13a1446bf83f3fcdda2))


### Build System

* **nixos-compatibility:** build system compatible with nixos ([#32](https://github.com/alemoreno991/BazelStarterTemplate/issues/32)) ([7b2718c](https://github.com/alemoreno991/BazelStarterTemplate/commit/7b2718cbbb39e8647a597e85ddb7ffcbfb4d5ad1))

## [1.3.0](https://github.com/alemoreno991/BazelStarterTemplate/compare/v1.2.0...v1.3.0) (2025-04-12)


### Features

* fake ([7604e6e](https://github.com/alemoreno991/BazelStarterTemplate/commit/7604e6eb97e726eacac528b7cf4a604cf1462b2e))
* fake ([0d22bb6](https://github.com/alemoreno991/BazelStarterTemplate/commit/0d22bb618686e9cd76ba2e83bf295a0ddd15fccd))

## [1.2.0](https://github.com/alemoreno991/BazelStarterTemplate/compare/v1.1.0...v1.2.0) (2025-04-12)


### Features

* fake ([1a073d9](https://github.com/alemoreno991/BazelStarterTemplate/commit/1a073d92ddbfd504e4f81459fb2f5321eb32566d))
* fake ([b7b3041](https://github.com/alemoreno991/BazelStarterTemplate/commit/b7b3041fa40239bef9df23336252b33f88715b5a))

## [1.1.0](https://github.com/alemoreno991/BazelStarterTemplate/compare/v1.0.0...v1.1.0) (2025-04-12)


### Features

* mimic a meaningful change to the code ([8a3b164](https://github.com/alemoreno991/BazelStarterTemplate/commit/8a3b16469058068ea4275c1adbd8d20ad83fa895))


### Bug Fixes

* Modify the release-please configuration ([29e696c](https://github.com/alemoreno991/BazelStarterTemplate/commit/29e696c51021d1b31f80a5d12cbfe52e63a017d3))
* **release-please:** fix release please config ([577b63c](https://github.com/alemoreno991/BazelStarterTemplate/commit/577b63c9611f3a9874cc62523f6540ce25e28fd2))

## 1.0.0 (2025-04-12)


### Bug Fixes

* Documentation quick fix ([58c24ab](https://github.com/alemoreno991/BazelStarterTemplate/commit/58c24abd3613ac6c2ffaea5ced92b8d84c70499c))
* release-please write access to issues ([3de0aa3](https://github.com/alemoreno991/BazelStarterTemplate/commit/3de0aa3a5814c65779db416b8d9a42637b03606c))
