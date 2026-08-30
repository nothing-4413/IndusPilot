# Design

Replace stale statements with the implemented boundary: MongoDB stores AI interaction documents only, while MySQL remains the system of record for transactional and audit data. Document that MongoDB mode coordinates indexes at startup, exposes bounded repository metrics, and uses an authenticated bounded `ping` for required readiness checks. Keep future work limited to explicitly unimplemented non-structured data use cases rather than describing the existing AI interaction adapter as absent.
