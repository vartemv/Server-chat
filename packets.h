//
// Created by artem on 4/14/24.
//

#ifndef IPK_SERVER_PACKETS_H

#define IPK_SERVER_PACKETS_H

#include <cstring>
#include <cstdint>
#include <string>

#endif //IPK_SERVER_PACKETS_H


typedef struct Packets {
    uint8_t MessageType;
    uint16_t MessageID;

    Packets(uint8_t type, uint16_t id) {
        MessageType = type;
        MessageID = id;
    }

    virtual int construct_message(uint8_t *b) {
        memcpy(b, &this->MessageType, sizeof(this->MessageType));
        b += sizeof(this->MessageType);


        //uint16_t ID = htons(this->MessageID);
        uint16_t ID = this->MessageID;
        memcpy(b, &ID, sizeof(ID));
        b += sizeof(ID);
        return 3;
    }

} Packet;

typedef struct ConfirmPackets : public Packets {

    uint16_t Ref_MessageID;

    ConfirmPackets(uint8_t type, uint16_t id, uint16_t ref_id) : Packets(type, id) {
        Ref_MessageID = ref_id;
    }

    int construct_message(uint8_t *b) override {
        memcpy(b, &this->MessageType, sizeof(this->MessageType));
        b += sizeof(this->MessageType);

        uint16_t ID = this->Ref_MessageID;
        memcpy(b, &ID, sizeof(ID));
        b += sizeof(ID);
        return sizeof(this->MessageType) + sizeof(ID);
    }

} ConfirmPacket;

typedef struct JoinPackets : public Packets {

    std::string ChannelID;
    std::string DisplayName;

    JoinPackets(uint8_t type, uint16_t id, std::string ch_id, std::string disp_name) : Packets(type, id) {
        ChannelID = std::move(ch_id);
        DisplayName = std::move(disp_name);
    }

    int construct_message(uint8_t *b) override {
        memcpy(b, &this->MessageType, sizeof(this->MessageType));
        b += sizeof(this->MessageType);

        //uint16_t ID = htons(this->MessageID);
        uint16_t ID = this->MessageID;
        memcpy(b, &ID, sizeof(ID));
        b += sizeof(ID);

        memcpy(b, ChannelID.c_str(), ChannelID.length());
        b[ChannelID.length()] = '\0';
        b += ChannelID.length() + 1;

        memcpy(b, DisplayName.c_str(), DisplayName.length());
        b[DisplayName.length()] = '\0';
        b += DisplayName.length() + 1;
        return sizeof(this->MessageType) + sizeof(ID) + ChannelID.length() + 1 + DisplayName.length() + 1;
    }

} JoinPacket;

// To create ERR use MsgPackets struct
typedef struct MsgPackets : public Packets {

    std::string MessageContents;
    std::string DisplayName;

    MsgPackets(uint8_t type, uint16_t id, std::string content, std::string disp_name) : Packets(type, id) {
        MessageContents = std::move(content);
        DisplayName = std::move(disp_name);
    }

    int construct_message(uint8_t *b) override {
        memcpy(b, &this->MessageType, sizeof(this->MessageType));
        b += sizeof(this->MessageType);

        //uint16_t ID = htons(this->MessageID);
        uint16_t ID = this->MessageID;
        memcpy(b, &ID, sizeof(ID));
        b += sizeof(ID);

        memcpy(b, DisplayName.c_str(), DisplayName.length());
        b[DisplayName.length()] = '\0';
        b += DisplayName.length() + 1;

        memcpy(b, MessageContents.c_str(), MessageContents.length());
        b[MessageContents.length()] = '\0';
        b += MessageContents.length() + 1;
        return sizeof(this->MessageType) + sizeof(ID) + DisplayName.length() + 1 + MessageContents.length() + 1;
    }

} MsgPacket;

typedef struct AuthPackets : public Packets {

    std::string Username;
    std::string DisplayName;
    std::string Secret;

    AuthPackets(uint8_t type, uint16_t id, std::string u_n, std::string disp_name, std::string sec) : Packets(type,
                                                                                                              id) {
        Username = std::move(u_n);
        DisplayName = std::move(disp_name);
        Secret = std::move(sec);
    }

    int construct_message(uint8_t *b) override {
        memcpy(b, &this->MessageType, sizeof(this->MessageType));
        b += sizeof(this->MessageType);
        //std::cout<<this->MessageType<<std::endl;

        //uint16_t ID = htons(this->MessageID);
        uint16_t ID = this->MessageID;
        memcpy(b, &ID, sizeof(ID));
        b += sizeof(ID);

        memcpy(b, Username.c_str(), Username.length());
        b[Username.length()] = '\0';
        b += Username.length() + 1;

        memcpy(b, DisplayName.c_str(), DisplayName.length());
        b[DisplayName.length()] = '\0';
        b += DisplayName.length() + 1;

        memcpy(b, Secret.c_str(), Secret.length());
        b[Secret.length()] = '\0';
        b += Secret.length() + 1;
        return sizeof(this->MessageType) + sizeof(ID) + Username.length() + 1 + DisplayName.length() + 1 +
               Secret.length() + 1;
    }

} AuthPacket;

typedef struct ReplyPackets : public Packets {
    std::string Message;
    uint8_t result;
    uint16_t ref_id;

    ReplyPackets(uint8_t type, uint16_t id, std::string mes, uint8_t res, uint16_t ref) : Packets(type, id) {
        Message = std::move(mes);
        result = res;
        ref_id = ref;
    }

    int construct_message(uint8_t *b) override {
        memcpy(b, &this->MessageType, sizeof(this->MessageType));
        b += sizeof(this->MessageType);
        uint16_t ID = this->MessageID;
        memcpy(b, &ID, sizeof(ID));
        b += sizeof(ID);

        memcpy(b, &result, sizeof(result));
        b += sizeof(result);

        memcpy(b, &ref_id, sizeof(ref_id));
        b += sizeof(ref_id);

        memcpy(b, Message.c_str(), Message.length());
        b[Message.length()] = '\0';
        b += Message.length() + 1;

        return sizeof(this->MessageType) + sizeof(ID) + sizeof(result) + sizeof(ref_id) + Message.length() + 1;
    }
} ReplyPacket;