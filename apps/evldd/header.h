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

        uint8_t     type() const;
        uint32_t    id() const;
        uint16_t    body_type() const;
        uint32_t    body_size() const;
        uint8_t     ext_count() const;
        uint16_t    ext_len() const;
    private:
        const char* buf_;
    };

    class Builder {
    public:
        Builder(char* buf);

        void Build();
        
        void set_type(uint8_t type);
        void set_id(uint32_t id);
        void set_body_type(uint16_t body_type);
        void set_body_size(uint32_t body_size);
        void set_ext_count(uint8_t ext_count);
        void set_ext_len(uint16_t ext_len);
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
