#ifndef SESSIONINFO_H_INCLUDED
#define SESSIONINFO_H_INCLUDED

class SessionInfo {
public:
    QString name;
    int day_of_week; /* 1 = Monday, 7 = Sunday, to work with QDate */
    int id;
    int order_number;
    int start_minutes;

SessionInfo() : name(), day_of_week(0), id(-1), order_number(0), start_minutes(-1) {}
    
};


#endif /* ifndef SESSIONINFO_H_INCLUDED */
