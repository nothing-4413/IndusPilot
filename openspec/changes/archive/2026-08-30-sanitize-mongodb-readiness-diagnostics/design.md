# Design

Keep the MongoDB probe's existing structured failure prefixes and success messages. In the exception path, sanitize URI userinfo patterns in the driver message, cap the remaining diagnostic to a small fixed size, and return it through the existing `MongoProbeResult` without throwing. The probe will never log or return the original URI.
