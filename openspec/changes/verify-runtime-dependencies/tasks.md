# 任务清单

## 1. OpenSpec
- [x] 1.1 编写运行依赖预检提案和规格增量
- [x] 1.2 OpenSpec 严格校验通过

## 2. MySQL schema 版本
- [x] 2.1 增加 `schema_migrations` 版本表
- [x] 2.2 在现有 MySQL 脚本中登记执行版本
- [x] 2.3 保持脚本可重复执行

## 3. 部署前预检
- [x] 3.1 新增 `deployment/preflight.ps1`
- [x] 3.2 检查配置文件、MySQL 脚本、版本登记和 compose 依赖
- [x] 3.3 输出中文检查结果和失败原因

## 4. 文档、验证与提交
- [x] 4.1 更新部署和生产就绪文档
- [x] 4.2 运行预检、默认测试、`dev-http` 测试和 OpenSpec 校验
- [x] 4.3 按模块提交 Git 记录
