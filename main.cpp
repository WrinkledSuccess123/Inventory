#include <iostream>
#include <map>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <thread>
#include <sstream>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

map<string, string> category_labels = {
    {"R", "Resistors"},
    {"C", "Capacitors"},
    {"E", "Electrolytic Capacitors"},
    {"D", "Diodes"},
    {"T", "Transistors"},
    {"P", "Potentiometers"},
    {"S", "Switches"},
    {"I", "Integrated Circuits"},
    {"X", "Miscellaneous"}
};

// --- Clear Console ---
void clear_console() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// --- ASCII Banner ---
void print_banner() {
    cout << R"(

 ███████╗██╗  ██╗██╗████████╗     ██╗   ██╗███████╗ █████╗ ██╗  ██╗
 ██╔════╝██║  ██║██║╚══██╔══╝     ╚██╗ ██╔╝██╔════╝██╔══██╗██║  ██║
 ███████╗███████║██║   ██║         ╚████╔╝ █████╗  ███████║███████║
 ╚════██║██╔══██║██║   ██║          ╚██╔╝  ██╔══╝  ██╔══██║██╔══██║
 ███████║██║  ██║██║   ██║           ██║   ███████╗██║  ██║██║  ██║
 ╚══════╝╚═╝  ╚═╝╚═╝   ╚═╝           ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝

        JACK LARKINS SUPER SICK ELECTRONICS COMPONENT INVENTORY

)";
}

// --- Convert suffixes to numbers ---
double parse_value(const string& str) {
    string numeric;
    char suffix = '\0';
    for (char ch : str) {
        if (isdigit(ch) || ch == '.') numeric += ch;
        else if (suffix == '\0') suffix = ch;
    }

    double multiplier = 1.0;
    switch (suffix) {
        case 'p': multiplier = 1e-12; break;
        case 'n': multiplier = 1e-9; break;
        case 'u': multiplier = 1e-6; break;
        case 'm': multiplier = 1e-3; break;
        case 'r': multiplier = 1; break;
        case 'k': multiplier = 1e3; break;
        case 'M': multiplier = 1e6; break;
        default:  multiplier = 1; break;
    }

    try {
        return stod(numeric) * multiplier;
    } catch (...) {
        return 0.0;
    }
}

// --- Timestamp + Logging ---
string current_time() {
    auto now = chrono::system_clock::now();
    time_t time = chrono::system_clock::to_time_t(now);
    string t = ctime(&time);
    t.pop_back();
    return t;
}

void log_action(const string& message) {
    ofstream log("history.log", ios::app);
    log << "[" << current_time() << "] " << message << "\n";
}

// --- Component Struct ---
struct Component {
    string category;
    string value;
    int quantity;

    json to_json() const {
        return json{{"category", category}, {"value", value}, {"quantity", quantity}};
    }

    static Component from_json(const json& j) {
        return Component{
            j.value("category", ""),
            j.value("value", ""),
            j.value("quantity", 0)
        };
    }
};

// --- Inventory Manager ---
class Inventory {
private:
    vector<Component> items;
    string filename;

public:
    Inventory(const string& file) : filename(file) {
        load();
    }

    void load() {
        ifstream in(filename);
        if (in) {
            json j;
            try {
                in >> j;
                for (const auto& item : j) {
                    items.push_back(Component::from_json(item));
                }
            } catch (...) {
                cerr << "Error: Could not parse JSON inventory.\n";
            }
        }
    }

    void save() const {
        json j;
        for (const auto& item : items) {
            j.push_back(item.to_json());
        }
        ofstream out(filename);
        out << j.dump(4);
    }

    void add_item() {
        Component c;
        cout << "Enter category (R/C/E/D/T/P/S/I/X): ";
        getline(cin, c.category);
        cout << "Enter value (e.g. 10k, 100nF): ";
        getline(cin, c.value);
        cout << "Enter quantity: ";
        cin >> c.quantity;
        cin.ignore();

        for (auto& item : items) {
            if (item.category == c.category && item.value == c.value) {
                item.quantity += c.quantity;
                log_action("Updated: [" + item.category + "] " + item.value + " | +" + to_string(c.quantity));
                cout << "Component exists — updated quantity.\n";
                save();
                return;
            }
        }

        items.push_back(c);
        log_action("Added: [" + c.category + "] " + c.value + " | +" + to_string(c.quantity));
        cout << "Component added.\n";
        save();
    }

    void list_items() const {
        cout << "\n--- Full Inventory ---\n";
        for (const auto& c : items) {
            cout << "[" << c.category << "] " << c.value << " | Qty: " << c.quantity << "\n";
        }
    }

    void list_by_category() const {
        string cat;
        cout << "Enter category to list (R/C/E/D/T/P/S/I/X): ";
        getline(cin, cat);
        if (cat.empty()) {
            cout << "No category entered.\n";
            return;
        }

        enum class SortMode { VALUE, QUANTITY };
        SortMode mode = SortMode::VALUE;

        while (true) {
            clear_console();
            string label = category_labels.count(cat) ? category_labels[cat] : cat;
            cout << "--- " << label << " [Sorting: " << (mode == SortMode::VALUE ? "value" : "quantity") << "] ---\n";

            vector<Component> filtered;
            for (const auto& c : items) {
                if (c.category == cat) filtered.push_back(c);
            }

            if (filtered.empty()) {
                cout << "(none found)\n";
            } else {
                if (mode == SortMode::VALUE) {
                    sort(filtered.begin(), filtered.end(), [](const Component& a, const Component& b) {
                        return parse_value(a.value) < parse_value(b.value);
                    });
                } else {
                    sort(filtered.begin(), filtered.end(), [](const Component& a, const Component& b) {
                        return a.quantity > b.quantity;
                    });
                }

                for (const auto& c : filtered) {
                    cout << c.value << " | Qty: " << c.quantity << "\n";
                }
            }

            cout << "\nPress [S] to toggle sort mode, or [Enter] to return...\n> ";
            string input;
            getline(cin, input);
            if (input.empty()) break;
            if (input == "s" || input == "S") {
                mode = (mode == SortMode::VALUE) ? SortMode::QUANTITY : SortMode::VALUE;
            }
        }
    }

    void reduce_or_remove_item() {
        string cat, val;
        cout << "Enter category (R/C/E/D/T/P/S/I/X): ";
        getline(cin, cat);
        cout << "Enter value: ";
        getline(cin, val);

        for (auto it = items.begin(); it != items.end(); ++it) {
            if (it->category == cat && it->value == val) {
                cout << "Found: " << it->value << " | Qty: " << it->quantity << "\n";
                cout << "Enter quantity to remove (or full to delete): ";
                int qty;
                cin >> qty;
                cin.ignore();

                if (qty >= it->quantity) {
                    log_action("Removed: [" + it->category + "] " + it->value + " | Full delete");
                    items.erase(it);
                    cout << "Removed entire component.\n";
                } else {
                    it->quantity -= qty;
                    log_action("Reduced: [" + it->category + "] " + it->value + " | -" + to_string(qty));
                    cout << "Reduced quantity. New qty: " << it->quantity << "\n";
                }

                save();
                return;
            }
        }

        cout << "Component not found.\n";
    }

    void bar_chart_by_value() const {
        string cat;
        cout << "Enter category to chart (R/C/E/D/T/P/S/I/X): ";
        getline(cin, cat);

        map<string, int> value_counts;
        for (const auto& c : items) {
            if (c.category == cat) {
                value_counts[c.value] += c.quantity;
            }
        }

        if (value_counts.empty()) {
            cout << "No components found in that category.\n";
            return;
        }

        size_t max_label_len = 0;
        int max_qty = 0;
        for (const auto& [value, qty] : value_counts) {
            max_label_len = max(max_label_len, value.length());
            max_qty = max(max_qty, qty);
        }

        const int bar_width = 30;
        string label = category_labels.count(cat) ? category_labels[cat] : cat;
        cout << "\n--- [" << label << "] Inventory Breakdown ---\n";
        for (const auto& [value, qty] : value_counts) {
            int bar_len = static_cast<int>((static_cast<double>(qty) / max_qty) * bar_width);
            cout << setw(max_label_len) << value << " | ";
            for (int i = 0; i < bar_len; ++i) cout << "█";
            cout << " (" << qty << ")\n";
        }
    }

    void show_history() {
        ifstream log("history.log");
        if (!log) {
            cout << "No history yet.\n";
            return;
        }

        cout << "\n--- Inventory History Log ---\n";
        string line;
        while (getline(log, line)) {
            cout << line << "\n";
        }
    }

    void check_bom_csv() const {
        string filename;
        cout << "Enter path to CSV file: ";
        getline(cin, filename);

        ifstream file(filename);
        if (!file) {
            cout << "Failed to open file.";
            return;
        }

        struct Requirement {
            string category;
            string value;
            int quantity;
        };

        vector<Requirement> required_parts;

        string line;
        getline(file, line); // Skip header

        while (getline(file, line)) {
            stringstream ss(line);
            string reference, value, datasheet, footprint, qty_str, dnp;
            getline(ss, reference, ',');
            getline(ss, value, ',');
            getline(ss, datasheet, ',');
            getline(ss, footprint, ',');
            getline(ss, qty_str, ',');
            getline(ss, dnp, ',');

            if (!dnp.empty()) continue; // Skip Do Not Populate

            // Infer category
            string category = "X";
            if (footprint.find("Capacitor") != string::npos) category = "C";
            else if (footprint.find("CP_Radial") != string::npos) category = "E";
            else if (footprint.find("Diode") != string::npos) category = "D";
            else if (footprint.find("Transistor") != string::npos) category = "T";
            else if (footprint.find("Potentiometer") != string::npos) category = "P";
            else if (footprint.find("Resistor") != string::npos) category = "R";
            else if (footprint.find("Switch") != string::npos) category = "S";
            else if (footprint.find("IC") != string::npos || footprint.find("SOIC") != string::npos) category = "I";

            try {
                int qty = stoi(qty_str);
                required_parts.push_back({category, value, qty});
            } catch (...) {
                cout << "Invalid line or quantity: " << line << "\n";
            }
        }

        bool all_ok = true;
        vector<Requirement> missing;

        for (const auto& req : required_parts) {
            int owned_qty = 0;
            for (const auto& item : items) {
                if (item.category == req.category && item.value == req.value) {
                    owned_qty = item.quantity;
                    break;
                }
            }

            if (owned_qty < req.quantity) {
                all_ok = false;
                missing.push_back({req.category, req.value, req.quantity - owned_qty});
            }
        }

        if (all_ok) {
            cout << "\n✅ You have everything you need!\n";
        } else {
            cout << "\n❌ Missing or insufficient components:\n";
            for (const auto& m : missing) {
                cout << "[" << m.category << "] " << m.value << " | Need: " << m.quantity << " more\n";
            }
        }
    }

    void run() {
        while (true) {
            clear_console();
            cout << "Options:\n"
                 << "1. List all\n"
                 << "2. List by category\n"
                 << "3. Add component\n"
                 << "4. Remove or reduce component\n"
                 << "5. View history log\n"
                 << "6. View per-category chart\n"
                 << "7. Check required parts from CSV\n"
                 << "8. Exit\n> ";
            int choice;
            cin >> choice;
            cin.ignore();

            clear_console();
            if (choice == 1) list_items();
            else if (choice == 2) list_by_category();
            else if (choice == 3) add_item();
            else if (choice == 4) reduce_or_remove_item();
            else if (choice == 5) show_history();
            else if (choice == 6) bar_chart_by_value();
            else if (choice == 7) check_bom_csv();
            else break;

            cout << "\nPress Enter to continue...";
            cin.ignore();
        }
    }
};

// --- Main ---
int main() {
    clear_console();
    print_banner();
    cout << "\nInitializing local database...\n";
    this_thread::sleep_for(chrono::milliseconds(3000));
    cout << "Ready.\n\n";

    Inventory inv("inventory.json");
    inv.run();
    return 0;
}
