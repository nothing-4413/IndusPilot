const database = db.getSiblingDB('induspilot');

function assertSmoke(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function collectionExists(name) {
  return database.getCollectionNames().includes(name);
}

assertSmoke(collectionExists('operation_logs'), 'operation_logs collection missing');
assertSmoke(collectionExists('ai_interactions'), 'ai_interactions collection missing');
assertSmoke(collectionExists('diagnostic_documents'), 'diagnostic_documents collection missing');

const smokeId = 'db-smoke-mongodb-001';
const occurredAt = new Date('2026-08-01T00:00:00Z');

database.operation_logs.updateOne(
  { eventCode: smokeId },
  {
    $set: {
      relatedType: 'dependency-smoke',
      relatedId: smokeId,
      level: 'info',
      message: 'MongoDB 真实文档 CRUD 集成测试',
      updatedAt: new Date()
    },
    $setOnInsert: { createdAt: occurredAt }
  },
  { upsert: true }
);

const operationLog = database.operation_logs.findOne({ eventCode: smokeId });
assertSmoke(operationLog && operationLog.relatedType === 'dependency-smoke', 'operation log upsert failed');

database.ai_interactions.updateOne(
  { interactionCode: 'db-smoke-mongodb-ai-001' },
  {
    $set: {
      relatedType: 'operation_log',
      relatedId: smokeId,
      prompt: 'summarize dependency smoke result',
      response: 'requires operator review',
      updatedAt: new Date()
    },
    $setOnInsert: { createdAt: occurredAt }
  },
  { upsert: true }
);

assertSmoke(
  database.ai_interactions.countDocuments({ interactionCode: 'db-smoke-mongodb-ai-001', relatedId: smokeId }) === 1,
  'AI interaction document upsert failed'
);

database.diagnostic_documents.updateOne(
  { documentCode: 'db-smoke-mongodb-doc-001' },
  {
    $set: {
      title: '数据库依赖 smoke 文档',
      content: '验证 MongoDB collection、索引和文档 CRUD 能在 CI 中重复执行。',
      tags: ['dependency-smoke', 'mongodb'],
      updatedAt: new Date()
    },
    $setOnInsert: { createdAt: occurredAt }
  },
  { upsert: true }
);

assertSmoke(
  database.diagnostic_documents.countDocuments({ documentCode: 'db-smoke-mongodb-doc-001', tags: 'mongodb' }) === 1,
  'diagnostic document upsert failed'
);

const operationIndexes = database.operation_logs.getIndexes().map((index) => index.name);
const aiIndexes = database.ai_interactions.getIndexes().map((index) => index.name);
const documentIndexes = database.diagnostic_documents.getIndexes().map((index) => index.name);
assertSmoke(operationIndexes.includes('relatedType_1_relatedId_1_createdAt_-1'), 'operation_logs index missing');
assertSmoke(aiIndexes.includes('relatedType_1_relatedId_1_createdAt_-1'), 'ai_interactions index missing');
assertSmoke(documentIndexes.includes('title_text_content_text'), 'diagnostic_documents text index missing');

print('mongodb_real_crud_smoke_passed');
