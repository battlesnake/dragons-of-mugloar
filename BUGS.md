# Client is using undocumented `encrypted` field in messages

Assuming that `null` means "plain-text" and `1` means "string fields are base64 encoded".


# Client is using undocumented `probability` field in messages

Assuming it's an enum representing likelihood of mission completing successfully (i.e. get gold, don't lose a life).


# Base64 parser (for "encrypted" messages) performs no input validation and returns no error codes

This needs to change at some point.


# Some responses are not defined correctly in the API spec

Field types (some defined a `string` when they should be `number`/`bool`).

Message structures (defined as object with array member, when actually the entire response is an array).

