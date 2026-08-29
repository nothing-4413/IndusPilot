# 任务清单

## 1. OpenSpec
- [x] 1.1 编写凭据生命周期的 proposal、design、spec 和 tasks

## 2. 密码与仓储边界
- [x] 2.1 增加密码策略配置、解析、校验和随机盐 PBKDF2 哈希生成
- [x] 2.2 扩展内存/MySQL 用户仓储的密码哈希更新接口
- [x] 2.3 增加 IdentityService 密码轮换结果和审计所需的安全错误分类

## 3. HTTP 与验证
- [x] 3.1 增加认证密码轮换 HTTP route，确保明文密码不进入日志/响应
- [x] 3.2 增加基础单测和 HTTP smoke，覆盖成功、当前密码错误、策略失败和存储失败

## 4. 文档与交付
- [x] 4.1 更新配置示例、部署文档和生产 readiness 边界
- [x] 4.2 运行 dev/dev-http 全量门禁并归档 change
- [x] 4.3 按独立阶段提交并推送 GitHub
