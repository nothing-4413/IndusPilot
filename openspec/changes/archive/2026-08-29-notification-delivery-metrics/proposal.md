# Change: Expose notification delivery metrics

## Why

Notification queue state is persisted and auditable, but operators cannot observe delivery outcomes by channel without querying every notification row. This delays detection of a failing webhook or an unsupported channel configuration.

## What Changes

- Record each notification delivery outcome in the shared metrics registry.
- Export bounded channel and outcome labels through `/metrics`.
- Cover sent, retrying, dead-letter, and unsupported outcomes without exposing target URLs or messages.
- Add regression coverage and operator documentation.

## Non-Goals

- No change to queue retry timing or delivery behavior.
- No notification target, alert id, rule id, or message in metric labels.
