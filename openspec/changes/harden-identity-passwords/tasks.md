# 任务清单

## 1. OpenSpec
- [x] 1.1 编写身份密码生产化提案和规格增量
- [x] 1.2 OpenSpec 严格校验通过

## 2. 密码校验边界
- [x] 2.1 增加版本化 PBKDF2-SHA256 密码校验模块
- [x] 2.2 保留 `plain:` 和历史明文开发数据兼容
- [x] 2.3 将 `IdentityService` 登录和认证接入统一校验器

## 3. 种子数据与文档
- [x] 3.1 将 MySQL 演示账号改为 PBKDF2 哈希
- [x] 3.2 补齐 MySQL operator/maintainer 演示账号和角色关系
- [x] 3.3 更新部署与生产就绪文档

## 4. 测试与提交
- [x] 4.1 补充密码校验单测覆盖
- [x] 4.2 默认构建、`dev-http` 测试和 OpenSpec 严格校验通过
- [x] 4.3 按模块提交 Git 记录
