# Design

`interactionCode` is the repository upsert filter, so it is the collection's persistence identity. A named unique ascending index enforces this invariant at the database boundary. MongoDB index creation is idempotent when the index options and key pattern match.

The integration smoke first verifies the index exists and is unique, then attempts a conflicting insert and requires MongoDB to reject it. Existing upsert and related-object sort indexes remain unchanged.
