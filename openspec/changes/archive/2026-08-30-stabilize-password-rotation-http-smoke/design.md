# Design

Introduce a named credential-operation timeout in the HTTP smoke script and use it for successful password rotation, rotation restore, and the two logins that verify the new credentials. This changes only test waiting behavior; server-side password policy, hashing, and HTTP semantics remain unchanged.
