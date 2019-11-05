include "statistics_log.h"
void update_statistic(p_sta statistic){
  statistic->average_wait_time_landing=statistics->sum_wait_time_landing/statistics->landed_flights;
  statistic->average_wait_time_taking_of=statistics->sum_wait_time_taking_of/statistics->take_of_flights;
  statistic->average_number_holds=statistics->sum_number_holds/statistics->total_holds;
  statistic->average_number_holds_urgency=statistics->sum_number_holds_urgency/statistics->total_holds_urgency;
}
