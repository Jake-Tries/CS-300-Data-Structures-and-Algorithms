//============================================================================
// Name        : ProjectTwo.cpp
// Author      : Jacob Barker
// Course      : CS 300
// Professor   : Bryant Moscon
// Date        : October 20, 2025
// Version     : 1.0
// Description : Project Two - ABCU Advising Assistance Program
//               Implements a hash table to manage and display course data
//============================================================================


#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

// Utilities 

static inline string trim(const string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static inline string upperId(string s) {
    for (char& ch : s) ch = static_cast<char>(toupper(static_cast<unsigned char>(ch)));
    return s;
}

// Split by comma (no quoted fields in project input)
static vector<string> splitCSV(const string& line) {
    vector<string> out;
    string token;
    stringstream ss(line);
    while (getline(ss, token, ',')) out.push_back(trim(token));
    return out;
}

// Course + Hash Table 

struct Course {
    string number;              // courseNumber (key)
    string title;               // courseName
    vector<string> prerequisites; // list of courseNumbers
};

class HashTable {
  private:
    struct Node {
        Course data;
        Node* next;
        Node(const Course& c) : data(c), next(nullptr) {}
    };

    // A modest prime bucket count keeps chains short for this data size.
    static constexpr size_t DEFAULT_BUCKETS = 251;

    vector<Node*> buckets;

    // Simple multiplicative string hash (deterministic)
    size_t hashKey(const string& key) const {
        unsigned long long h = 0ULL;
        for (unsigned char ch : key) {
            h = h * 131ULL + ch; // 131 is common multiplier for strings
        }
        return static_cast<size_t>(h % buckets.size());
    }

  public:
    explicit HashTable(size_t bucketCount = DEFAULT_BUCKETS)
        : buckets(bucketCount, nullptr) {}

    ~HashTable() { clear(); }

    void clear() {
        for (Node*& head : buckets) {
            Node* cur = head;
            while (cur) {
                Node* nxt = cur->next;
                delete cur;
                cur = nxt;
            }
            head = nullptr;
        }
    }

    // Insert or update by key
    void insert(const Course& c) {
        size_t idx = hashKey(c.number);
        Node* head = buckets[idx];
        // Update in-place if exists
        for (Node* cur = head; cur; cur = cur->next) {
            if (cur->data.number == c.number) {
                cur->data = c;
                return;
            }
        }
        // Prepend new node (O(1))
        Node* node = new Node(c);
        node->next = head;
        buckets[idx] = node;
    }

    // Return pointer to Course or nullptr
    const Course* find(const string& number) const {
        size_t idx = hashKey(number);
        for (Node* cur = buckets[idx]; cur; cur = cur->next) {
            if (cur->data.number == number) return &cur->data;
        }
        return nullptr;
    }

    // Gather all courses for sorting/printing
    vector<Course> toVector() const {
        vector<Course> out;
        out.reserve(300); // small hint; not required
        for (Node* head : buckets) {
            for (Node* cur = head; cur; cur = cur->next) {
                out.push_back(cur->data);
            }
        }
        return out;
    }
};

// Loader 

struct LoadResult {
    bool ok = false;
    size_t validLines = 0;
    size_t errors = 0;
};

LoadResult loadCoursesFromFile(const string& filePath, HashTable& table) {
    table.clear();

    ifstream in(filePath);
    if (!in) {
        cerr << "Error: cannot open file \"" << filePath << "\"." << endl;
        return {false, 0, 1};
    }

    // First pass: read all lines, collect course IDs and basic format errors
    vector<string> lines;
    lines.reserve(256);

    string line;
    while (getline(in, line)) {
        string t = trim(line);
        if (!t.empty()) lines.push_back(t);
    }
    in.close();

    if (lines.empty()) {
        cerr << "Error: file contains no data." << endl;
        return {false, 0, 1};
    }

    unordered_set<string> allIds;
    size_t errors = 0;

    for (const string& ln : lines) {
        vector<string> tokens = splitCSV(ln);
        if (tokens.size() < 2) {
            cerr << "Format error: line has fewer than 2 fields -> \"" << ln << "\"" << endl;
            ++errors;
            continue;
        }
        allIds.insert(upperId(tokens[0]));
    }

    // Second pass: validate each prerequisite and build Course objects
    size_t valid = 0;
    for (const string& ln : lines) {
        vector<string> tokens = splitCSV(ln);
        if (tokens.size() < 2) {
            // already counted error above
            continue;
        }

        Course c;
        c.number = upperId(tokens[0]);
        c.title  = tokens[1];
        for (size_t i = 2; i < tokens.size(); ++i) {
            string pr = upperId(tokens[i]);
            if (allIds.find(pr) == allIds.end()) {
                cerr << "Validation error: prerequisite \"" << pr
                     << "\" for course " << c.number << " not found in file." << endl;
                ++errors;
            }
            c.prerequisites.push_back(pr);
        }

        table.insert(c);
        ++valid;
    }

    return {errors == 0, valid, errors};
}

// Printing 

void printCourse(const HashTable& table, const string& rawId) {
    string id = upperId(trim(rawId));
    const Course* c = table.find(id);
    if (!c) {
        cout << "Course " << id << " not found." << endl;
        return;
    }

    cout << c->number << ", " << c->title << '\n';

    if (c->prerequisites.empty()) {
        cout << "Prerequisites: none" << '\n';
        return;
    }

    cout << "Prerequisites: ";
    for (size_t i = 0; i < c->prerequisites.size(); ++i) {
        const string& pid = c->prerequisites[i];
        const Course* pc = table.find(pid);
        // Print number and (optional) title if available
        if (pc) cout << pc->number << " (" << pc->title << ")";
        else    cout << pid;
        if (i + 1 < c->prerequisites.size()) cout << ", ";
    }
    cout << '\n';
}

void printCourseList(const HashTable& table) {
    vector<Course> all = table.toVector();
    // Sort alphanumerically by course number (case-insensitive already uppercase)
    sort(all.begin(), all.end(),
         [](const Course& a, const Course& b) { return a.number < b.number; });

    cout << "Here is a sample schedule:" << '\n';
    for (const Course& c : all) {
        cout << c.number << ", " << c.title << '\n';
    }
}

// Menu 

void printMenu() {
    cout << '\n';
    cout << "1. Load Data Structure." << '\n';
    cout << "2. Print Course List." << '\n';
    cout << "3. Print Course." << '\n';
    cout << "9. Exit" << '\n';
    cout << "What would you like to do? ";
}

int main() {
    cout << "Welcome to the course planner." << '\n';

    HashTable table;
    bool loaded = false;
    string lastFile;

    while (true) {
        printMenu();

        int choice = 0;
        if (!(cin >> choice)) {
            // Handle non-numeric input gracefully
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a menu option." << endl;
            continue;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // consume remainder of the line

        if (choice == 1) {
            cout << "Enter file name (e.g., ABCU_Advising_Program_Input.txt): ";
            string path;
            getline(cin, path);
            path = trim(path);

            LoadResult r = loadCoursesFromFile(path, table);
            if (r.ok) {
                cout << r.validLines << " courses loaded successfully." << endl;
                loaded = true;
                lastFile = path;
            } else {
                cout << "Load completed with " << r.errors
                     << " error(s). Fix the input and try again." << endl;
                loaded = (r.validLines > 0); // still allow partial work if desired
                lastFile = path;
            }
        }
        else if (choice == 2) {
            if (!loaded) {
                cout << "Please load the data structure first (Option 1)." << endl;
            } else {
                printCourseList(table);
            }
        }
        else if (choice == 3) {
            if (!loaded) {
                cout << "Please load the data structure first (Option 1)." << endl;
            } else {
                cout << "What course do you want to know about? ";
                string id;
                getline(cin, id);
                printCourse(table, id);
            }
        }
        else if (choice == 9) {
            cout << "Thank you for using the course planner!" << endl;
            break;
        }
        else {
            cout << choice << " is not a valid option." << endl;
        }
    }

    return 0;
}
