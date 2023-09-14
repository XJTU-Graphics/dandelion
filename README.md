![dandelion_logo](./resources/icons/dandelion_64.png)

# Dandelion 3D

## Introduction

Dandelion is a light-weight, cross-platform graphics framework for educational usage. It allow users to load and place objects in a 3D scene, render the scene using different offline renderers, edit mesh with the help of halfedge data structure, or perform simple simulation. We believe it will be a good choice for basic computer graphics courses.

We have Dandelion Development Documents published on both *GitHub Pages* and *Read the Docs*:

- https://xjtu-graphics.github.io/Dandelion-docs/index.html
- https://dandelion-docs.readthedocs.io

The two websites are synchronized automatically, please visit any of them to get more information about Dandelion 3D. We hope the documents will be helpful for extending Dandelion to more graphics labs.

## Channels

There are two channels, the **dev** channel and the **release** channel.

The dev channel is the full functional version, containing all source code (including the solutions of labs). It is only accessible for members of XJTU Graphics. We develop and test new features on the dev channel, Then port them to the release channel.

The release channel is a public repository (XJTU-Graphics/dandelion) containing the skeleton code for Computer Graphics course of XJTU (Xi'an Jiaotong University). Some of implementations are removed, waiting for students to fill them. If you have found any bug or have any suggestion about Dandelion, please open an issue on the release channel to tell us. Pull requests are welcomed, but not all PRs will be merged. We will decide whether to merge a PR or not by its source code quality and our development plan. Once a PR is merged to the release channel, we will port it to the dev channel.

Please not create any PR providing solution of any lab, such a PR will never be merged and will be deleted once we have noticed it. If your issue benefits Dandelion or your PR is merged, your name (or nick name) will be recorded in the list of contributors. Thanks for all supports, suggestions, tests and our students. Without your help, Dandelion will never be successful.


## Contributors

- greyishsong: Architecture, OpenGL API encapsulation, geometry processing and collision detection
- JoTaiLang: Software renderers and BVH acceleration
- siyuanluo: Kinetic movement solution
- ibm5100: Testing on macOS
- Yqy123kkxx: Validation of labs about simulation
- STORM-S314: Validation of labs about geometry processing
- Kylee: Validation of labs about rendering

---

## 简介

Dandelion 是一个轻量级的跨平台图形学实验框架，实现了场景布局、离线渲染、网格编辑、物理模拟等功能。对于计算机图形学基础课程来说，我们相信它会是一个不错的实验框架。

Dandelion 开发者文档同步托管于 GitHub Pages 和 Read the Docs 两个平台：

- https://xjtu-graphics.github.io/Dandelion-docs/index.html
- https://dandelion-docs.readthedocs.io

请访问上述二者之一来获取详细的文档。无论你想要完成实验还是拓展 Dandelion 的功能，这些文档应该都有所帮助。

## 频道

我们维护了 dev 和 release 两个频道，分别用于内部开发和对外发布。

dev 频道是功能完善的版本，包含了 Dandelion 的所有源代码。只有西安交通大学图形学团队成员才能访问其中的代码，我们在这个频道上开发并测试新的功能和特性，然后将相应的更新转移到 release 频道上。

release 频道是一个公开仓库 (XJTU-Graphics/dandelion) ，包含西安交通大学图形学课程实验的框架代码。对应于实验题目的部分功能实现被删去，留待参与课程的同学自行填写。如果你发现任何 bug 或对 Dandelion 有什么建议，请在这个公开仓库中提出一个 issue 。我们也欢迎各位提出 Pull Request ，我们会根据 PR 是否符合我们的更新规划以及所提交代码的质量决定是否合并。当我们在 release 频道上合并了你的 PR ，我们会将其移植到 dev 频道。

请勿提出提供实验答案的 PR ，这样的 PR 一经发现立即删除。如果你的 issue 对 Dandelion 的发展有益或我们合并了你的 PR ，我们会将你记录至贡献者列表中。感谢所有改进框架、提出建议或进行测试的朋友们，也感谢各位参与实验的同学们。没有你们的帮助，Dandelion 定然无法走到今天这一步。

## 贡献者

- greyishsong, 架构设计和 OpenGL API 封装、几何处理、碰撞响应
- JoTaiLang, 软渲染器、BVH 加速结构
- siyuanluo, 运动求解
- ibm5100, macOS 平台测试
- Yqy123kkxx, 物理模拟部分实验验证
- STORM-S314, 几何部分实验验证
- Kylee, 渲染部分实验验证
