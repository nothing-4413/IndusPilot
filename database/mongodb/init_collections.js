db = db.getSiblingDB('induspilot');

db.createCollection('operation_logs');
db.createCollection('ai_interactions');
db.createCollection('diagnostic_documents');

db.operation_logs.createIndex({ relatedType: 1, relatedId: 1, createdAt: -1 });
db.ai_interactions.createIndex({ relatedType: 1, relatedId: 1, createdAt: -1 });
db.diagnostic_documents.createIndex({ title: 'text', content: 'text' });