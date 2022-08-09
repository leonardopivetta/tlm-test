#include "gps.h"

int Gps::id_ = 0;

std::string Gps::get_header(std::string separator)
{
    std::stringstream ss;
    ss << "_timestamp" + separator
       << "msg_type" + separator
       << "time" + separator
       << "latitude" + separator
       << "longitude" + separator
       << "altitude" + separator
       << "fix" + separator
       << "satellites" + separator
       << "fix_state" + separator
       << "age_of_correction" + separator
       << "course_over_ground_degrees" + separator
       << "course_over_ground_degrees_magnetic" + separator
       << "speed_kmh" + separator
       << "mode" + separator
       << "position_diluition_precision" + separator
       << "horizontal_diluition_precision" + separator
       << "vertical_diluition_precision" + separator

       << "heading_valid" + separator
       << "heading_vehicle" + separator
       << "heading_motion" + separator
       << "heading_accuracy_estimate" + separator
       << "speed_accuracy" + separator;
    return ss.str();
}
std::string Gps::get_string(std::string separator)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(9)
       << data.timestamp << separator
       << data.msg_type + separator
       << data.time + separator
       << data.latitude << separator
       << data.longitude << separator
       << data.altitude << separator
       << data.fix << separator
       << data.satellites << separator
       << data.fix_state + separator
       << data.age_of_correction << separator
       << data.course_over_ground_degrees << separator
       << data.course_over_ground_degrees_magnetic << separator
       << data.speed_kmh << separator
       << data.mode + separator
       << data.position_diluition_precision << separator
       << data.horizontal_diluition_precision << separator
       << data.vertical_diluition_precision << separator

       << data.heading_valid << separator
       << data.heading_vehicle << separator
       << data.heading_motion << separator
       << data.heading_accuracy_estimate << separator
       << data.speed_accuracy << separator;
    return ss.str();
}

void Gps::serialize(devices::Gps *gps)
{
    gps->set_timestamp(data.timestamp);
    gps->set_msg_type(data.msg_type);
    gps->set_time(data.time);
    gps->set_latitude(data.latitude);
    gps->set_longitude(data.longitude);
    gps->set_altitude(data.altitude);
    gps->set_fix(data.fix);
    gps->set_satellites(data.satellites);
    gps->set_fix_state(data.fix_state);
    gps->set_age_of_correction(data.age_of_correction);
    gps->set_course_over_ground_degrees(data.course_over_ground_degrees);
    gps->set_course_over_ground_degrees_magnetic(data.course_over_ground_degrees_magnetic);
    gps->set_speed_kmh(data.speed_kmh);
    gps->set_mode(data.mode);
    gps->set_position_diluition_precision(data.position_diluition_precision);
    gps->set_horizontal_diluition_precision(data.horizontal_diluition_precision);
    gps->set_vertical_diluition_precision(data.vertical_diluition_precision);

    gps->set_heading_valid(data.heading_valid);
    gps->set_heading_vehicle(data.heading_vehicle);
    gps->set_heading_motion(data.heading_motion);
    gps->set_heading_accuracy_estimate(data.heading_accuracy_estimate);
    gps->set_speed_accuracy(data.speed_accuracy);
}
std::string Gps::get_readable()
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3)
       << name << "\n"
       << "\t_timestamp -> \t" << data.timestamp << "\n"
       << "\tmsg_type -> \t" << data.msg_type << "\n"
       << "\ttime -> \t" << data.time << "\n"
       << "\tlatitude -> \t" << data.latitude << "\n"
       << "\tlongitude -> \t" << data.longitude << "\n"
       << "\taltitude -> \t" << data.altitude << "\n"
       << "\tfix -> \t" << data.fix << "\n"
       << "\tsatellites -> \t" << data.satellites << "\n"
       << "\tfix_state -> \t" << data.fix_state << "\n"
       << "\tage_of_correction -> \t" << data.age_of_correction << "\n"
       << "\tcourse_over_ground_degrees -> \t" << data.course_over_ground_degrees << "\n"
       << "\tcourse_over_ground_degrees_magnetic -> \t" << data.course_over_ground_degrees_magnetic << "\n"
       << "\tspeed_kmh -> \t" << data.speed_kmh << "\n"
       << "\tmode -> \t" << data.mode << "\n"
       << "\tposition_diluition_precision -> \t" << data.position_diluition_precision << "\n"
       << "\thorizontal_diluition_precision -> \t" << data.horizontal_diluition_precision << "\n"
       << "\tvertical_diluition_precision -> \t" << data.vertical_diluition_precision << "\n"

       << "\theading_valid -> \t" << data.heading_valid << "\n"
       << "\theading_vehicle -> \t" << data.heading_vehicle << "\n"
       << "\theading_motion -> \t" << data.heading_motion << "\n"
       << "\theading_accuracy_estimate -> \t" << data.heading_accuracy_estimate << "\n"
       << "\tspeed_accuracy -> \t" << data.speed_accuracy << "\n";
    return ss.str();
}

bool parse_ubx_line(const string &line, UBX_MSG_PVT &msg)
{
    UBX_MSG_MATCH match{0x01, 0x07, sizeof(UBX_MSG_PVT)};

    const uint8_t *buff = (uint8_t *)line.c_str();
    int idx = -1;
    for (int i = 0; i < line.size(); i++)
    {
        if (buff[i] == 0xb5 && buff[i + 1] == 0x62)
        {
            if (buff[i + 2] == match.msgClass && buff[i + 3] == match.msgID)
            {
                uint16_t sz = (buff[i + 5] << 8) | buff[i + 4];
                if (sz == (match.length))
                {
                    idx = i + 2;
                }
            }
        }
    }
    if (idx == -1)
        return false;

    // Checksum
    uint8_t a = 0;
    uint8_t b = 0;
    for (int i = 0; i < (sizeof(UBX_MSG_MATCH) + sizeof(UBX_MSG_PVT)); i++)
    {
        a += buff[i + idx];
        b += a;
    }
    // Check of checksum
    if (a != buff[sizeof(UBX_MSG_MATCH) + sizeof(UBX_MSG_PVT) + idx] ||
        b != buff[sizeof(UBX_MSG_MATCH) + sizeof(UBX_MSG_PVT) + idx + 1])
        return false;

    msg = *(UBX_MSG_PVT *)(&buff[idx + sizeof(UBX_MSG_MATCH)]);

    return true;
}

uint16_t reverse16(const uint16_t &in)
{
    uint16_t a = ((in >> 8) & 0xFF);
    a |= (in & 0xFF) << 8;
    return a;
}

uint32_t reverse32(const uint32_t &in)
{
    uint32_t a = ((in >> 24) & 0xFF);
    a |= ((in >> 16) & 0xFF) << 8;
    a |= ((in >> 8) & 0xFF) << 16;
    a |= (in & 0xFF) << 24;
    return a;
}

int16_t reversei16(const int16_t &in)
{
    int16_t a = ((in >> 8) & 0xFF);
    a |= (in & 0xFF) << 8;
    return a;
}
int32_t reversei32(const int32_t &in)
{
    int32_t a = ((in >> 24) & 0xFF);
    a |= ((in >> 16) & 0xFF) << 8;
    a |= ((in >> 8) & 0xFF) << 16;
    a |= (in & 0xFF) << 24;
    return a;
}

ParseError parse_gps(Gps *gps_, const uint64_t &timestamp, string &line)
{

    UBX_MSG_PVT msg;
    // if (parse_ubx_line(line, msg))
    // {
    //     gps_->clear();
    //     gps_->data.timestamp = timestamp;
    //     gps_->data.msg_type = "UBX";
    //     gps_->data.latitude = ((double)msg.lat) * 10e-8;
    //     gps_->data.longitude = ((double)msg.lon) * 10e-8;
    //     // From this message the coord are kinda different to check
    //     // gps_->data.latitude = 0.0;
    //     // gps_->data.longitude = 0.0;

    //     gps_->data.altitude = msg.hMSL * 10e-4;
    //     gps_->data.speed_kmh = msg.gSpeed * 3.6 * 10e-4;
    //     gps_->data.heading_motion = ((double)msg.headMot) * 10e-6;
    //     gps_->data.speed_accuracy = msg.sAcc * 3.6 * 10e-4;
    //     gps_->data.heading_accuracy_estimate = ((double)msg.headAcc) * 10e-6;
    //     gps_->data.position_diluition_precision = ((double)msg.pDOP) * 0.01;
    //     gps_->data.age_of_correction = (uint8_t)(msg.flags3 & 0b0000000000011110);
    //     gps_->data.heading_vehicle = ((double)msg.headVeh) * 10e-6;
    //     return ParseError::ParseOk;
    // }

    auto idx = line.find('$');
    if (idx == string::npos)
        return ParseError::NoMatch;
    if (idx > 0)
        line = line.substr(idx, line.size() - idx);
    if (line[0] != '$')
        return ParseError::NoMatch;

    vector<string> s_line = split(line, ',');

    if (s_line[0].size() != 6)
        return ParseError::NoMatch;

    // remove $GP or $GN
    s_line[0].erase(0, 3);

    if (s_line[0] == "GGA")
    {
        if (s_line.size() != 15)
            return ParseError::MessageLength;
        // check if needed fileds are empty
        int ret = empty_fields(s_line, vector<int>{1, 2, 4, 6, 7, 9});
        if (ret != -1)
            return ParseError::MessageEmpty;

        gps_->clear();

        gps_->data.timestamp = timestamp;
        gps_->data.msg_type = "GGA";

        gps_->data.time = s_line[1];
        gps_->data.latitude = stod(s_line[2]) / 100.0;
        gps_->data.longitude = stod(s_line[4]) / 100.0;
        gps_->data.fix = stoi(s_line[6]);
        gps_->data.satellites = stoi(s_line[7]);
        gps_->data.horizontal_diluition_precision = stod(s_line[8]);
        gps_->data.fix_state = FIX_STATE[gps_->data.fix];
        gps_->data.altitude = stod(s_line[9]);
        if (s_line[13] != "")
        {
            gps_->data.age_of_correction = stod(s_line[13]);
        }

        return ParseError::ParseOk;
    }
    else if (s_line[0] == "VTG")
    {
        if (s_line.size() != 10)
            return ParseError::MessageLength;

        gps_->clear();

        gps_->data.timestamp = timestamp;
        gps_->data.msg_type = "VTG";

        if (s_line[1] != "")
            gps_->data.course_over_ground_degrees = stod(s_line[1]);
        else
            gps_->data.course_over_ground_degrees = 0.0;
        if (s_line[3] != "")
            gps_->data.course_over_ground_degrees_magnetic = stod(s_line[3]);
        else
            gps_->data.course_over_ground_degrees_magnetic = 0.0;
        if (s_line[7] != "")
            gps_->data.speed_kmh = stod(s_line[7]);
        else
            gps_->data.speed_kmh = 0.0;

        return ParseError::ParseOk;
    }
    else if (s_line[0] == "GSA")
    {
        if (s_line.size() != 19)
            return ParseError::MessageLength;

        gps_->clear();

        gps_->data.timestamp = timestamp;
        gps_->data.msg_type = "GSA";
        gps_->data.mode = FIX_MODE[stoi(s_line[2]) - 1];
        gps_->data.position_diluition_precision = stod(s_line[15]);
        gps_->data.horizontal_diluition_precision = stod(s_line[16]);
        gps_->data.vertical_diluition_precision = stod(s_line[17]);
        return ParseError::ParseOk;
    }

    return ParseError::NoMatch;
}