// Demo + a small CLI: `./bank-demo` runs the scripted scenario,
// `./bank-demo statement [account-id] [YYYY-MM]` prints a monthly
// statement for that account (default account ACC-1001, default month
// the current month).
#include <cctype>
#include <iostream>
#include <string>

#include "bank.h"

using bank::Account;
using bank::Bank;
using bank::Cents;

static void print_statement(const Account& account) {
    std::cout << "Account " << account.id() << " (" << account.owner() << ")\n";
    std::cout << "  Balance: " << Account::format(account.balance_cents()) << "\n";
    for (const auto& tx : account.history()) {
        std::cout << "  " << tx.type << " " << Account::format(tx.amount_cents)
                  << " -> " << Account::format(tx.balance_after_cents);
        if (!tx.note.empty()) std::cout << "  (" << tx.note << ")";
        std::cout << "\n";
    }
}

static void print_monthly_statement(const Account& account, int year, int month) {
    const bank::MonthlyStatement stmt = account.monthly_statement(year, month);
    std::cout << "Monthly statement — " << account.id() << " (" << account.owner()
              << ") — " << stmt.year << '-';
    if (stmt.month < 10) std::cout << '0';
    std::cout << stmt.month << "\n";
    std::cout << "  Opening balance: " << Account::format(stmt.opening_balance_cents)
              << "\n";
    if (stmt.entries.empty()) {
        std::cout << "  No transactions this month.\n";
    }
    for (const auto& entry : stmt.entries) {
        std::cout << "  " << entry.date << "  " << entry.type << "  ";
        if (entry.amount_cents >= 0) std::cout << '+';
        std::cout << Account::format(entry.amount_cents) << "  -> "
                  << Account::format(entry.balance_after_cents) << "\n";
    }
    std::cout << "  Total deposits:    " << Account::format(stmt.total_deposits_cents)
              << "\n";
    std::cout << "  Total withdrawals: "
              << Account::format(stmt.total_withdrawals_cents) << "\n";
    std::cout << "  Closing balance:   "
              << Account::format(stmt.closing_balance_cents) << "\n";
}

// The scripted scenario behind both the demo and the statement command.
static void seed_scenario(Bank& bank) {
    bank.create_account("ACC-1001", "Ayesha Khan", 50000);   // $500.00
    bank.create_account("ACC-1002", "Bilal Ahmed", 25000);   // $250.00

    bank.find_account("ACC-1001")->deposit(12550, "salary");        // +$125.50
    bank.find_account("ACC-1001")->withdraw(4000, "groceries");     // -$40.00
    bank.transfer("ACC-1001", "ACC-1002", 10000);                   // $100.00
}

// Parses "YYYY-MM"; returns false for anything else. A string shaped like a
// month but with an out-of-range month is reported separately so a typo
// like 2026-13 is not silently treated as an account id.
static bool parse_year_month(const std::string& text, int& year, int& month,
                             bool& looks_like_month) {
    looks_like_month = false;
    if (text.size() != 7 || text[4] != '-') return false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i == 4) continue;
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) return false;
    }
    looks_like_month = true;
    year = std::stoi(text.substr(0, 4));
    month = std::stoi(text.substr(5, 2));
    return month >= 1 && month <= 12;
}

static void print_usage() {
    std::cerr << "Usage:\n"
                 "  bank-demo                              run the demo scenario\n"
                 "  bank-demo statement [account-id] [YYYY-MM]\n"
                 "      print a monthly statement (default account ACC-1001,\n"
                 "      default month the current month)\n";
}

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "statement") {
        std::string account_id = "ACC-1001";
        int year = 0;
        int month = 0;
        for (int i = 2; i < argc; ++i) {
            int parsed_year = 0;
            int parsed_month = 0;
            bool looks_like_month = false;
            if (parse_year_month(argv[i], parsed_year, parsed_month,
                                 looks_like_month)) {
                year = parsed_year;
                month = parsed_month;
            } else if (looks_like_month) {
                std::cerr << "Invalid month: " << argv[i]
                          << " (expected YYYY-MM, month 01-12)\n";
                return 1;
            } else {
                account_id = argv[i];
            }
        }
        if (year == 0) {  // default: current month
            const std::string today = bank::current_date();
            year = std::stoi(today.substr(0, 4));
            month = std::stoi(today.substr(5, 2));
        }
        Bank bank;
        seed_scenario(bank);
        const Account* account = bank.find_account(account_id);
        if (account == nullptr) {
            std::cerr << "Unknown account: " << account_id << "\n";
            return 1;
        }
        print_monthly_statement(*account, year, month);
        return 0;
    }
    if (argc > 1) {
        print_usage();
        return 1;
    }

    Bank bank;
    seed_scenario(bank);

    if (!bank.find_account("ACC-1002")->withdraw(9999999, "too much")) {
        std::cout << "Overdraft correctly refused.\n\n";
    }

    print_statement(*bank.find_account("ACC-1001"));
    std::cout << "\n";
    print_statement(*bank.find_account("ACC-1002"));
    return 0;
}
