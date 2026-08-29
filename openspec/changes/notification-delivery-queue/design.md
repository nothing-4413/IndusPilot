# Design: Durable notification delivery queue

`AlertNotification` gains `nextAttemptAtUnixMs`, `leaseUntilUnixMs`, `leaseToken`, and `maxAttempts`. The repository exposes `claimDueNotifications(now, leaseMs, limit, leaseToken)`; MySQL claims rows with a conditional update and then reads only rows carrying that token. In-memory storage performs the same transition under its mutex.

Dispatch creates a random worker token, claims due `queued`/`retrying` rows, and delivers only the claimed batch. A failed attempt schedules `retrying` at 1, 2, 4, ... seconds; once `maxAttempts` is reached the status becomes `dead_letter`. Successful delivery clears lease data. Manual retry resets the schedule but never resets the attempt counter, preserving an auditable limit.

Migration `013_notification_delivery_queue.sql` adds the fields idempotently and indexes due queue scans. Legacy rows default to immediately due with a three-attempt limit. Notification IDs remain the idempotency key through existing upsert behavior.
