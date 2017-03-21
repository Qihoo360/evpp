
Another ping pong benchmark test

<pre>
    // Decode deserializes data (as Buffer*) to this message
    // message format:
    // [x][x][x][x][x][x][x][x][x][x][x][x]...
    // |(int32)   ||(int32)   || (binary)
    // |4-byte    ||4-byte    || N-byte
    // |body size ||packet num|| msg body
</pre>

The first 4 bytes is header, and following data body.
