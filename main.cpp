#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <set>
#include <sstream>

using namespace std;

struct Submission {
    char problem;
    string status;
    int time;
};

struct ProblemStatus {
    int wrong_before_freeze = 0;
    int wrong_after_freeze = 0;
    int solved_time = -1;
    bool frozen = false;
};

struct Team {
    string name;
    map<char, ProblemStatus> problems;
    vector<Submission> submissions;
    int solved_count = 0;
    int penalty_time = 0;
    vector<int> solve_times;
    
    void update_stats(bool include_frozen) {
        solved_count = 0;
        penalty_time = 0;
        solve_times.clear();
        
        for (auto& p : problems) {
            if (p.second.solved_time != -1 && (!p.second.frozen || include_frozen)) {
                solved_count++;
                int wrong_count = p.second.wrong_before_freeze;
                if (include_frozen) {
                    wrong_count += p.second.wrong_after_freeze;
                }
                penalty_time += p.second.solved_time + 20 * wrong_count;
                solve_times.push_back(p.second.solved_time);
            }
        }
        sort(solve_times.rbegin(), solve_times.rend());
    }
};

struct TeamComparator {
    bool operator()(const Team* a, const Team* b) const {
        if (a->solved_count != b->solved_count) {
            return a->solved_count > b->solved_count;
        }
        if (a->penalty_time != b->penalty_time) {
            return a->penalty_time < b->penalty_time;
        }
        int n = min(a->solve_times.size(), b->solve_times.size());
        for (int i = 0; i < n; i++) {
            if (a->solve_times[i] != b->solve_times[i]) {
                return a->solve_times[i] < b->solve_times[i];
            }
        }
        return a->name < b->name;
    }
};

map<string, Team> teams;
vector<Team*> scoreboard;
bool competition_started = false;
bool is_frozen = false;
int duration_time;
int problem_count;

void flush_scoreboard() {
    for (auto& p : teams) {
        p.second.update_stats(false);
    }
    
    scoreboard.clear();
    for (auto& p : teams) {
        scoreboard.push_back(&p.second);
    }
    
    sort(scoreboard.begin(), scoreboard.end(), TeamComparator());
}

string get_problem_display(const ProblemStatus& ps) {
    if (ps.frozen) {
        if (ps.wrong_before_freeze == 0) {
            return "0/" + to_string(ps.wrong_after_freeze);
        } else {
            return "-" + to_string(ps.wrong_before_freeze) + "/" + to_string(ps.wrong_after_freeze);
        }
    } else if (ps.solved_time != -1) {
        if (ps.wrong_before_freeze == 0) {
            return "+";
        } else {
            return "+" + to_string(ps.wrong_before_freeze);
        }
    } else {
        if (ps.wrong_before_freeze == 0) {
            return ".";
        } else {
            return "-" + to_string(ps.wrong_before_freeze);
        }
    }
}

void print_scoreboard() {
    for (size_t i = 0; i < scoreboard.size(); i++) {
        Team* t = scoreboard[i];
        cout << t->name << " " << (i + 1) << " " << t->solved_count << " " << t->penalty_time;
        for (int p = 0; p < problem_count; p++) {
            char prob = 'A' + p;
            cout << " " << get_problem_display(t->problems[prob]);
        }
        cout << "\n";
    }
}

int main() {
    string line;
    
    while (getline(cin, line)) {
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        
        if (cmd == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            
            if (competition_started) {
                cout << "[Error]Add failed: competition has started.\n";
            } else if (teams.count(team_name)) {
                cout << "[Error]Add failed: duplicated team name.\n";
            } else {
                teams[team_name].name = team_name;
                cout << "[Info]Add successfully.\n";
            }
        } else if (cmd == "START") {
            if (competition_started) {
                cout << "[Error]Start failed: competition has started.\n";
            } else {
                string duration_str, problem_str;
                iss >> duration_str >> duration_time >> problem_str >> problem_count;
                competition_started = true;
                cout << "[Info]Competition starts.\n";
            }
        } else if (cmd == "SUBMIT") {
            char problem;
            string by, team_name, with, status, at;
            int time;
            iss >> problem >> by >> team_name >> with >> status >> at >> time;
            
            Team& t = teams[team_name];
            t.submissions.push_back({problem, status, time});
            
            ProblemStatus& ps = t.problems[problem];
            
            if (status == "Accepted") {
                if (ps.solved_time == -1) {
                    ps.solved_time = time;
                    if (is_frozen) {
                        ps.frozen = true;
                        ps.wrong_after_freeze++;
                    }
                }
                // Submissions after a problem is solved don't count
            } else {
                if (ps.solved_time == -1) {
                    if (is_frozen) {
                        ps.frozen = true;
                        ps.wrong_after_freeze++;
                    } else {
                        ps.wrong_before_freeze++;
                    }
                }
                // Wrong submissions after solving don't count towards penalty
            }
        } else if (cmd == "FLUSH") {
            flush_scoreboard();
            cout << "[Info]Flush scoreboard.\n";
        } else if (cmd == "FREEZE") {
            if (is_frozen) {
                cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            } else {
                is_frozen = true;
                cout << "[Info]Freeze scoreboard.\n";
            }
        } else if (cmd == "SCROLL") {
            if (!is_frozen) {
                cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            } else {
                cout << "[Info]Scroll scoreboard.\n";
                
                // Flush before scrolling
                flush_scoreboard();
                print_scoreboard();
                
                // Scroll process
                while (true) {
                    bool found = false;
                    Team* target_team = nullptr;
                    char target_problem = 0;
                    
                    // Find lowest ranked team with frozen problems
                    for (int i = scoreboard.size() - 1; i >= 0; i--) {
                        Team* t = scoreboard[i];
                        for (int p = 0; p < problem_count; p++) {
                            char prob = 'A' + p;
                            if (t->problems[prob].frozen) {
                                if (!target_team || prob < target_problem) {
                                    target_team = t;
                                    target_problem = prob;
                                    found = true;
                                }
                            }
                        }
                        if (found) break;
                    }
                    
                    if (!found) break;
                    
                    // Unfreeze the problem
                    int old_rank = 0;
                    for (size_t i = 0; i < scoreboard.size(); i++) {
                        if (scoreboard[i] == target_team) {
                            old_rank = i;
                            break;
                        }
                    }
                    
                    target_team->problems[target_problem].frozen = false;
                    if (target_team->problems[target_problem].solved_time != -1) {
                        target_team->problems[target_problem].wrong_before_freeze += target_team->problems[target_problem].wrong_after_freeze - 1;
                    } else {
                        target_team->problems[target_problem].wrong_before_freeze += target_team->problems[target_problem].wrong_after_freeze;
                    }
                    target_team->problems[target_problem].wrong_after_freeze = 0;
                    
                    // Recalculate and resort
                    flush_scoreboard();
                    
                    int new_rank = 0;
                    for (size_t i = 0; i < scoreboard.size(); i++) {
                        if (scoreboard[i] == target_team) {
                            new_rank = i;
                            break;
                        }
                    }
                    
                    if (new_rank < old_rank) {
                        cout << target_team->name << " " << scoreboard[new_rank + 1]->name << " " 
                             << target_team->solved_count << " " << target_team->penalty_time << "\n";
                    }
                }
                
                print_scoreboard();
                is_frozen = false;
            }
        } else if (cmd == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            
            if (!teams.count(team_name)) {
                cout << "[Error]Query ranking failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query ranking.\n";
                if (is_frozen) {
                    cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
                }
                
                int rank = 0;
                if (scoreboard.empty()) {
                    // Before first flush, use lexicographic order
                    vector<string> team_names;
                    for (auto& p : teams) {
                        team_names.push_back(p.first);
                    }
                    sort(team_names.begin(), team_names.end());
                    // cerr << "DEBUG: Looking for " << team_name << " in: ";
                    // for (auto& n : team_names) cerr << n << " ";
                    // cerr << endl;
                    for (size_t i = 0; i < team_names.size(); i++) {
                        if (team_names[i] == team_name) {
                            rank = i + 1;
                            break;
                        }
                    }
                } else {
                    for (size_t i = 0; i < scoreboard.size(); i++) {
                        if (scoreboard[i]->name == team_name) {
                            rank = i + 1;
                            break;
                        }
                    }
                }
                
                cout << team_name << " NOW AT RANKING " << rank << "\n";
            }
        } else if (cmd == "QUERY_SUBMISSION") {
            string team_name;
            iss >> team_name;
            
            if (!teams.count(team_name)) {
                cout << "[Error]Query submission failed: cannot find the team.\n";
                continue;
            }
            
            // Parse WHERE clause
            string where_str;
            iss >> where_str; // WHERE
            
            string condition;
            getline(iss, condition);
            
            // Parse PROBLEM= and STATUS=
            size_t problem_pos = condition.find("PROBLEM=");
            size_t and_pos = condition.find(" AND ");
            size_t status_pos = condition.find("STATUS=");
            
            string problem_filter = condition.substr(problem_pos + 8, and_pos - problem_pos - 8);
            string status_filter = condition.substr(status_pos + 7);
            
            cout << "[Info]Complete query submission.\n";
            
            Team& t = teams[team_name];
            bool found = false;
            Submission result;
            
            for (int i = t.submissions.size() - 1; i >= 0; i--) {
                Submission& s = t.submissions[i];
                bool match = true;
                
                if (problem_filter != "ALL" && s.problem != problem_filter[0]) {
                    match = false;
                }
                if (status_filter != "ALL" && s.status != status_filter) {
                    match = false;
                }
                
                if (match) {
                    result = s;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                cout << "Cannot find any submission.\n";
            } else {
                cout << team_name << " " << result.problem << " " << result.status << " " << result.time << "\n";
            }
        } else if (cmd == "END") {
            cout << "[Info]Competition ends.\n";
            break;
        }
    }
    
    return 0;
}