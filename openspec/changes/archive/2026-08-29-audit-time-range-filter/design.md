# Design: Operation audit time-range filters

`OperationAuditQuery` gains optional `occurredFrom` and `occurredTo` values. `AuditService` compares the stored canonical `YYYY-MM-DDTHH:MM:SS` timestamps lexicographically, which preserves chronological ordering for the existing format and keeps filtering consistent across memory and MySQL repositories.

The HTTP route parser accepts inclusive `occurredFrom` and `occurredTo` query parameters. It validates exactly 19 characters in the existing timestamp format, including calendar field ranges, and rejects `occurredFrom > occurredTo`. Both the events route and CSV export call this parser, so authorization, filtering, and error behavior stay aligned.
