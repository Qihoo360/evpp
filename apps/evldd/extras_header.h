namespace evldd {

class ExtrasHeader {
public:
    static const int kUnitSize = 2;
    static uint32_t size(int count) { return count * 2; }

    class Parser {
    public:
        Parser();

        void Parse(uint8_t ext_count, const char* buf);
        bool IsValid(uint16_t ext_len) const;
        bool GetExtItem(int index, uint8_t *type, uint8_t *len);
    private:
        uint8_t ext_count_;
        const char* ext_buf_;
    };

    class Builder {
    public:
        Builder(char* buf);

        void Build();
        void AddExtItem(uint8_t type, uint8_t len);

    private:
        int ext_count_;
        char* ext_buf_;
    };
};
class Extras {
public:
    class Parser {
    };
};
}
