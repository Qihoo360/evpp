namespace evldd {

/**
 * Header format (all in big-endian):
 *
 * | magic  |  type  |              msg-id               |    body-type    |
 * |--------|--------|--------|--------|--------|--------|--------|--------|
 *
 * |         body-length      | ext-n  |     ext-len     |    checksum     |
 * |--------|--------|--------|--------|--------|--------|--------|--------|
 *
 *
 *
 */
class Header {

public:
    enum Type {
        // Uplink type
        kRequest = 0,
        kPing,
        kCancel,

        // Downlink type
        kResponse = 128,
        kPong,
        kLast,
        kEnd,
    };

    class Parser {
    public:
        Parser();

        void Parse(const char* buf);
        bool IsValid() const;

        //TODO Type has been used asn a enum
        uint8_t     Type() const;
        uint32_t    Id() const;
        uint16_t    BodyType() const;
        uint32_t    BodySize() const;
        uint8_t     ExtCount() const;
        uint16_t    ExtSize() const;
    private:
        const char* buf_;
    };

    class Builder {
    public:
        Builder(char* buf);

        void Build();
        
        void SetType(uint8_t type);
        void SetId(uint32_t id);
        void SetBodyType(uint16_t body_type);
        void SetBodySize(uint32_t body_size);
        void SetExtCount(uint8_t ext_count);
        void SetExtSize(uint16_t ext_len);
    private:
        char* buf_;
    };
};


class Body {

public:
    class Parser {
    };
};

}
