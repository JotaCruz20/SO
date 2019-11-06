typedef struct{
    int created_flights,landed_flights,take_of_flighs;

    //average waiting times
    int sum_wait_time_landing,sum_wait_time_taking_of;
    double average_wait_time_landing,average_wait_time_taking_of;

    //average maneuvers number
    int total_holds,total_holds_urgency;
    int sum_number_holds,sum_number_holds_urgency;
    double average_number_holds,average_number_holds_urgency;

    int number_redirected_flights,rejected_flights;
}Statistic
typedef Statistic* p_sta;


//statistics functions
void update_statistic();

//log functions
void new_command(char* command);
char* current_time();
int verify_command(char* command);
