# Custom profile source bytes

All 26 runtime/host gates passed. Final source binding found that the Night JSON profile used CRLF locally while Git publishes LF. Standard media inputs already used exact LF bytes. Only Night is rebuilt after canonicalizing its profile, followed by its native distribution and maintenance gates; standard runtime gates remain bound to unchanged media.
