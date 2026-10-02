#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <filesystem>
#include <cctype>
#include <iomanip>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
namespace fs = std::filesystem;

// ===================== 颜色控制开关 =====================
bool USE_COLOR = true;

#ifdef _WIN32
bool enable_ansi_color() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return false;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
        return false;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode))
        return false;
    return true;
}
#endif

// 颜色宏，根据开关动态决定是否输出转义序列
#define RESET   (USE_COLOR ? "\033[0m" : "")
#define BOLD    (USE_COLOR ? "\033[1m" : "")
#define DIM     (USE_COLOR ? "\033[2m" : "")
#define RED     (USE_COLOR ? "\033[31m" : "")
#define GREEN   (USE_COLOR ? "\033[32m" : "")
#define YELLOW  (USE_COLOR ? "\033[33m" : "")
#define BLUE    (USE_COLOR ? "\033[34m" : "")
#define MAGENTA (USE_COLOR ? "\033[35m" : "")
#define CYAN    (USE_COLOR ? "\033[36m" : "")
#define WHITE   (USE_COLOR ? "\033[37m" : "")
#define HL      (USE_COLOR ? "\033[1;31;43m" : "")   // 黄底红字加粗，用于命中关键字

struct Note {
    string name;
    string time_str;
    string content;
};

vector<Note> notes;
string current_file = "note.txt";

// ---------- 字符串工具 ----------
string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

string to_lower(string s) {
    for (char &c : s)
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
}

// 大小写不敏感查找，返回首次匹配位置
size_t find_case_insensitive(const string &src, const string &key) {
    if (key.empty() || src.size() < key.size()) return string::npos;
    string src_low = to_lower(src);
    string key_low = to_lower(key);
    return src_low.find(key_low);
}

// 扫描当前目录，获取全部 .txt 笔记本文件名
vector<string> scan_all_notebooks() {
    vector<string> res;
    try {
        for (auto &entry: fs::directory_iterator(fs::current_path())) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                string name = entry.path().filename().string();
                string name_low = to_lower(name);
                if (name_low.substr(0,6) != "backup" && name != "CMakeLists.txt") {
                    res.push_back(name);
                }
            }
        }
    } catch (...) {
        // 读取目录出错直接返回空
    }
    return res;
}

// 获取当前时间字符串 YYYY-MM-DD HH:MM
string get_now_time() {
    time_t t = time(nullptr);
    tm *p = localtime(&t);
    char buf[32];
    sprintf(buf, "%04d-%02d-%02d %02d:%02d",
            p->tm_year + 1900,
            p->tm_mon + 1,
            p->tm_mday,
            p->tm_hour,
            p->tm_min);
    return string(buf);
}

// 判断一行是否像时间戳（YYYY-MM-DD HH:MM），用于兼容旧的两行笔记格式
bool is_time_line(const string &line) {
    if (line.size() < 16) return false;
    for (int i = 0; i < 4; i++) if (!isdigit(static_cast<unsigned char>(line[i]))) return false;
    if (line[4] != '-' || line[7] != '-') return false;
    if (!isdigit(static_cast<unsigned char>(line[5])) || !isdigit(static_cast<unsigned char>(line[6]))) return false;
    if (!isdigit(static_cast<unsigned char>(line[8])) || !isdigit(static_cast<unsigned char>(line[9]))) return false;
    if (line[10] != ' ') return false;
    if (!isdigit(static_cast<unsigned char>(line[11])) || !isdigit(static_cast<unsigned char>(line[12]))) return false;
    if (line[13] != ':') return false;
    if (!isdigit(static_cast<unsigned char>(line[14])) || !isdigit(static_cast<unsigned char>(line[15]))) return false;
    return true;
}

// 判断是否旧版本备份头部注释行，导入时跳过
bool is_old_backup_header(const string &line) {
    string t = trim(line);
    return t.rfind("====", 0) == 0;
}

// 从 ifstream 读取一条笔记，兼容新格式（名字+时间+内容）和旧格式（时间+内容）
// 自动跳过旧备份头部注释行
bool read_one_note(ifstream &fin, Note &n) {
    string first;
    while(getline(fin, first))
    {
        if(!is_old_backup_header(first)) break;
    }
    if (!fin) return false;

    if (is_time_line(first)) {
        n.name.clear();
        n.time_str = first;
        if (!getline(fin, n.content)) return false;
    } else {
        n.name = first;
        if (!getline(fin, n.time_str)) return false;
        if (!getline(fin, n.content)) return false;
    }
    return true;
}

void write_one_note(ofstream &fout, const Note &n) {
    fout << n.name << endl;
    fout << n.time_str << endl;
    fout << n.content << endl;
}

void load_notes() {
    ifstream fin(current_file);
    if (!fin.is_open()) return;
    Note n;
    while (read_one_note(fin, n)) notes.push_back(n);
    fin.close();
}

void save_notes() {
    ofstream fout(current_file);
    for (auto &item: notes) write_one_note(fout, item);
    fout.close();
}

vector<Note> read_notebook(const string &filename) {
    vector<Note> tmp_notes;
    ifstream fin(filename);
    if (!fin.is_open()) return tmp_notes;
    Note n;
    while (read_one_note(fin, n)) tmp_notes.push_back(n);
    fin.close();
    return tmp_notes;
}

// ---------- 各命令实现 ----------
void cmd_add() {
    string name, content;
    cout << YELLOW << "  名字 > " << RESET;
    getline(cin, name);
    cout << YELLOW << "  内容 > " << RESET;
    getline(cin, content);
    Note n;
    n.name = trim(name);
    n.time_str = get_now_time();
    n.content = content;
    notes.push_back(n);
    save_notes();
    cout << GREEN << "  [OK] 已保存" << RESET;
    if (!n.name.empty()) cout << BOLD << " \"" << n.name << "\"" << RESET;
    cout << "  (" << n.time_str << ")" << endl;
}

// ---------- 输出辅助：高亮 + 分页 ----------
const int PAGE_SIZE = 10;

// 把 text 输出到 cout，其中命中 key 的片段用高亮标注（key 为空则原样输出）
void highlight_text(const string &text, const string &key) {
    if (key.empty()) { cout << text; return; }
    string key_low = to_lower(key);
    string text_low = to_lower(text);
    size_t pos = 0;
    while (pos < text.size()) {
        size_t found = text_low.find(key_low, pos);
        if (found == string::npos) {
            cout << text.substr(pos);
            break;
        }
        cout << text.substr(pos, found - pos);
        cout << HL << text.substr(found, key.size()) << RESET;
        pos = found + key.size();
    }
}

// 输出一条笔记；num 为显示编号，key 非空时对名字和内容做关键字高亮
void print_note_line(int num, const Note &n, const string &key = "") {
    cout << "  " << BLUE << num << "." << RESET << " ";
    if (!n.name.empty()) {
        cout << BOLD << "【";
        highlight_text(n.name, key);
        cout << "】" << RESET << " ";
    }
    cout << DIM << "[" << n.time_str << "]" << RESET << " ";
    highlight_text(n.content, key);
    cout << endl;
}

// 每输出 PAGE_SIZE 条暂停一次；返回 false 表示用户按 q 中断
bool page_pause(int shown) {
    if (shown % PAGE_SIZE != 0) return true;
    cout << DIM << "\n  ── 已显示 " << shown << " 条，按 [ENTER] 继续，输入 q 退出 ──" << RESET;
    string s;
    if (!getline(cin, s)) return false;
    if (trim(s) == "q" || trim(s) == "Q") { cout << endl; return false; }
    cout << endl;
    return true;
}

void cmd_list() {
    cout << "\n" << BOLD << CYAN << "  ── 笔记列表" << RESET << "  " << DIM
         << "(" << notes.size() << " 条，当前: " << current_file << ")" << RESET << endl;
    if (notes.empty()) {
        cout << DIM << "  （空，使用 add 命令添加第一条笔记）" << RESET << endl;
        return;
    }
    for (size_t i = 0; i < notes.size(); i++) {
        print_note_line(static_cast<int>(i + 1), notes[i]);
        if (!page_pause(static_cast<int>(i + 1))) break;
    }
}

void cmd_delete(const string &args) {
    if (notes.empty()) {
        cout << YELLOW << "  当前笔记本为空，没有可删除的笔记。" << RESET << endl;
        return;
    }
    string num_str = args;
    if (num_str.empty()) {
        cout << YELLOW << "  输入要删除的编号 > " << RESET;
        getline(cin, num_str);
    }
    num_str = trim(num_str);
    int idx = 0;
    try {
        idx = stoi(num_str);
    } catch (...) {
        cout << RED << "  [ERROR] 编号格式错误" << RESET << endl;
        return;
    }
    if (idx < 1 || static_cast<size_t>(idx) > notes.size()) {
        cout << RED << "  [ERROR] 编号无效，范围 1-" << notes.size() << RESET << endl;
        return;
    }
    cout << GREEN << "  [OK] 已删除 #" << idx << RESET;
    if (!notes[idx - 1].name.empty()) cout << " 【" << notes[idx - 1].name << "】";
    cout << endl;
    notes.erase(notes.begin() + idx - 1);
    save_notes();
}

void cmd_revise(const string &args) {
    if (notes.empty()) {
        cout << YELLOW << "  当前笔记本为空。" << RESET << endl;
        return;
    }
    string num_str = args;
    if (num_str.empty()) {
        cout << YELLOW << "  输入要修改的编号 > " << RESET;
        getline(cin, num_str);
    }
    num_str = trim(num_str);
    int idx = 0;
    try {
        idx = stoi(num_str);
    } catch (...) {
        cout << RED << "  [ERROR] 编号格式错误" << RESET << endl;
        return;
    }
    if (idx < 1 || static_cast<size_t>(idx) > notes.size()) {
        cout << RED << "  [ERROR] 编号无效，范围 1-" << notes.size() << RESET << endl;
        return;
    }
    string new_name, new_text;
    cout << YELLOW << "  新名字（回车保持原名: "
         << (notes[idx - 1].name.empty() ? "（无）" : notes[idx - 1].name) << "）> " << RESET;
    getline(cin, new_name);
    if (!trim(new_name).empty())
        notes[idx - 1].name = trim(new_name);
    cout << YELLOW << "  新内容 > " << RESET;
    getline(cin, new_text);
    notes[idx - 1].content = new_text;
    save_notes();
    cout << GREEN << "  [OK] 已修改 #" << idx << "（创建时间保持不变）" << RESET << endl;
}

void cmd_search(const string &args) {
    string key = args;
    if (key.empty()) {
        cout << YELLOW << "  关键词（匹配名字或内容）> " << RESET;
        getline(cin, key);
    }
    key = trim(key);
    if (key.empty()) {
        cout << RED << "  [ERROR] 关键词不能为空。" << RESET << endl;
        return;
    }
    cout << "\n" << BOLD << MAGENTA << "  ── 搜索结果" << RESET << "  " << DIM
         << "关键词: " << HL << key << RESET << DIM << RESET << endl;
    int shown = 0;
    for (auto &item: notes) {
        bool name_match = find_case_insensitive(item.name, key) != string::npos;
        bool cont_match = find_case_insensitive(item.content, key) != string::npos;
        if (name_match || cont_match) {
            shown++;
            print_note_line(shown, item, key);
            if (!page_pause(shown)) break;
        }
    }
    if (shown == 0)
        cout << DIM << "  （没有匹配的笔记）" << RESET << endl;
    else
        cout << DIM << "\n  共 " << shown << " 条结果" << RESET << endl;
}

void cmd_clear() {
    string confirm;
    cout << YELLOW << "  确认清空 \"" << current_file << "\" 的全部笔记？输入 y 确认 > " << RESET;
    getline(cin, confirm);
    if (trim(confirm) == "y" || trim(confirm) == "Y") {
        notes.clear();
        save_notes();
        cout << GREEN << "  [OK] 已清空当前笔记本" << RESET << endl;
    } else {
        cout << DIM << "  已取消。" << RESET << endl;
    }
}

void cmd_switch() {
    cout << "\n" << BOLD << CYAN << "  ── 切换笔记本" << RESET << endl;
    vector<string> book_list = scan_all_notebooks();
    if (book_list.empty()) {
        cout << YELLOW << "  没有找到任何笔记本。" << RESET << endl;
        return;
    }
    for (size_t i = 0; i < book_list.size(); i++) {
        cout << "  " << BLUE << (i + 1) << "." << RESET << " " << book_list[i];
        if (book_list[i] == current_file) cout << GREEN << "  ← 当前" << RESET;
        cout << endl;
    }
    string sel_str;
    cout << YELLOW << "  输入序号 > " << RESET;
    getline(cin, sel_str);
    sel_str = trim(sel_str);
    int select;
    try {
        select = stoi(sel_str);
    } catch (...) {
        cout << RED << "  [ERROR] 输入不是有效数字" << RESET << endl;
        return;
    }
    if (select < 1 || static_cast<size_t>(select) > book_list.size()) {
        cout << RED << "  [ERROR] 无效选择，未切换。" << RESET << endl;
        return;
    }
    save_notes();
    notes.clear();
    current_file = book_list[select - 1];
    load_notes();
    cout << GREEN << "  [OK] 已切换到 " << BOLD << current_file << RESET << endl;
}

void cmd_export() {
    vector<string> book_list = scan_all_notebooks();
    if (book_list.empty()) {
        cout << RED << "  [ERROR] 没有找到任何笔记本可以导出！" << RESET << endl;
        return;
    }
    cout << "\n" << BOLD << CYAN << "  ── 选择要导出的笔记本" << RESET << endl;
    for (size_t i = 0; i < book_list.size(); i++)
        cout << "  " << BLUE << (i + 1) << "." << RESET << " " << book_list[i] << endl;
    cout << YELLOW << "  多个序号用空格分隔（例: 1 3）> " << RESET;
    string line;
    getline(cin, line);

    vector<int> selected_idx;
    string num;
    for (char ch: line) {
        if (ch == ' ') {
            if (!num.empty()) {
                try
                {
                    selected_idx.push_back(stoi(num));
                }
                catch(...)
                {
                    cout << YELLOW << "  [SKIP] 序号 \"" << num << "\" 不是有效数字" << RESET << endl;
                }
                num.clear();
            }
        } else num += ch;
    }
    if (!num.empty())
    {
        try
        {
            selected_idx.push_back(stoi(num));
        }
        catch(...)
        {
            cout << YELLOW << "  [SKIP] 序号 \"" << num << "\" 不是有效数字" << RESET << endl;
        }
    }

    for (int idx: selected_idx) {
        if (idx < 1 || static_cast<size_t>(idx) > book_list.size()) {
            cout << YELLOW << "  [SKIP] 序号 " << idx << " 无效" << RESET << endl;
            continue;
        }
        string book_name = book_list[idx - 1];
        vector<Note> book_data = read_notebook(book_name);
        string backup_name = "backup_" + book_name;
        ofstream fout(backup_name);
        // 修复：不再写入头部注释，备份文件格式和源文件完全一致
        for (auto &item: book_data) write_one_note(fout, item);
        fout.close();
        cout << GREEN << "  [OK] " << book_name << " -> " << backup_name << RESET << endl;
    }
}

void cmd_import() {
    string import_filename;
    cout << YELLOW << "  备份文件名（例: backup_note.txt）> " << RESET;
    getline(cin, import_filename);
    import_filename = trim(import_filename);
    ifstream fin(import_filename);
    if (!fin.is_open()) {
        cout << RED << "  [ERROR] 无法打开文件，请确认文件在程序同一目录。" << RESET << endl;
        return;
    }
    Note n;
    int import_count = 0;
    while (read_one_note(fin, n)) {
        notes.push_back(n);
        import_count++;
    }
    fin.close();
    save_notes();
    cout << GREEN << "  [OK] 导入 " << import_count << " 条笔记，已追加到 " << current_file << RESET << endl;
}

void cmd_create() {
    string new_name;
    cout << YELLOW << "  新笔记本名（直接写 xxx）> " << RESET;
    getline(cin, new_name);
    new_name = trim(new_name) + ".txt";
    save_notes();
    notes.clear();
    current_file = new_name;
    ofstream create_file(current_file);
    create_file.close();
    load_notes();
    cout << GREEN << "  [OK] 已新建并切换到 " << BOLD << current_file << RESET << endl;
}

void cmd_help() {
    // 命令列宽：纯 ASCII，setw 按字符数计 = 终端显示列数，严格对齐
    const int CMD_W = 26;
    cout << "\n" << BOLD << CYAN
         << "    ╔══════════════════════════════════════════════════╗"
         << "\n  ║                NoteBooks Pro  v2.0               ║"
         << "\n  ╚══════════════════════════════════════════════════╝" << RESET << endl;
    cout << "\n  " << BOLD << left << setw(CMD_W) << "命令" << RESET << BOLD << "说明" << RESET << endl;
    cout << "  " << DIM << left << setw(CMD_W + 4) << "───────────────────────────" << RESET << endl;
    auto row = [&](const char *cmd, const char *desc) {
        cout << "  " << GREEN << left << setw(CMD_W) << cmd << RESET << desc << endl;
    };
    row("add",                  "添加一条笔记（名字 + 内容），自动记录时间");
    row("list / ls",            "列出当前笔记本全部笔记");
    row("delete / rm <id>",     "按编号删除笔记（编号可直接跟在命令后）");
    row("revise / edit <id>",   "修改指定笔记的名字和内容（不改时间）");
    row("search / find <word>", "按关键词搜索名字或内容（大小写不敏感）");
    row("clear",                "清空当前笔记本（需确认）");
    row("switch",               "切换到其他笔记本");
    row("create",               "新建并切换到一个笔记本");
    row("export",               "批量导出笔记本为 backup_xxx.txt");
    row("import",               "导入备份文件，追加到当前笔记本");
    row("help / ?",             "显示本帮助");
    row("color off / on",       "开关终端颜色输出");
    row("exit / quit / q",      "保存并退出");
    cout << "\n  " << YELLOW << "提示：" << RESET << DIM
         << "命令不区分大小写；当前笔记本 = " << current_file << RESET << "\n" << endl;
}

void cmd_color(const string &args) {
    string opt = to_lower(trim(args));
    if(opt == "on")
    {
        USE_COLOR = true;
        cout << GREEN << "  [OK] 已开启颜色输出" << RESET << endl;
    }
    else if(opt == "off")
    {
        USE_COLOR = false;
        cout << GREEN << "  [OK] 已关闭颜色输出" << RESET << endl;
    }
    else
    {
        cout << YELLOW << "  当前颜色状态：" << (USE_COLOR ? "开启" : "关闭") << RESET << endl;
        cout << DIM << "  使用 color on / color off 切换" << RESET << endl;
    }
}

// ---------- 主程序 ----------
int main() {
#ifdef _WIN32
    enable_ansi_color();
#endif

    if (!fs::exists("note.txt")) {
        ofstream f("note.txt");
        f.close();
    }
    load_notes();

    // 启动横幅
    cout << "\n" << BOLD << CYAN;
    cout << "  ╔═══════════════════════════════════════════╗\n";
    cout << "  ║   Welcome NoteBooks Pro  v2.0             ║\n";
    cout << "  ║   A lightweight CLI notebook manager      ║\n";
    cout << "  ║   By Hant Korea                           ║\n";
    cout << "  ╚═══════════════════════════════════════════╝" << RESET << "\n\n";
    cout << "  当前笔记本: " << BOLD << BLUE << current_file << RESET << DIM
         << "  （共 " << notes.size() << " 条笔记）" << RESET << "\n";
    cout << "  颜色输出: ";
    if (USE_COLOR) cout << GREEN << "开启" << RESET;
    else           cout << DIM  << "关闭" << RESET;
    cout << "  输入 " << GREEN << "help" << RESET << "查看全部命令，"
         << GREEN << "exit" << RESET << " 退出。\n" << endl;

    string line;
    while (true) {
        // 命令提示符：notebooks(note.txt)>
        cout << BOLD << CYAN << "notebooks" << RESET
             << DIM << "(" << current_file << ")" << RESET
             << BOLD << "> " << RESET;

        if (!getline(cin, line)) { cout << endl; break; }
        line = trim(line);
        if (line.empty()) continue;

        // 取第一个 token 作为命令，其余作为参数
        size_t sp = line.find(' ');
        string cmd, args;
        if (sp == string::npos) {
            cmd = to_lower(line);
        } else {
            cmd = to_lower(trim(line.substr(0, sp)));
            args = trim(line.substr(sp + 1));
        }

        if (cmd == "add") {
            cmd_add();
        } else if (cmd == "list" || cmd == "ls") {
            cmd_list();
        } else if (cmd == "delete" || cmd == "del" || cmd == "rm") {
            cmd_delete(args);
        } else if (cmd == "revise" || cmd == "edit" || cmd == "rename") {
            cmd_revise(args);
        } else if (cmd == "search" || cmd == "find" || cmd == "grep") {
            cmd_search(args);
        } else if (cmd == "clear") {
            cmd_clear();
        } else if (cmd == "switch" || cmd == "use") {
            cmd_switch();
        } else if (cmd == "export") {
            cmd_export();
        } else if (cmd == "import") {
            cmd_import();
        } else if (cmd == "create" || cmd == "new") {
            cmd_create();
        } else if (cmd == "help" || cmd == "?") {
            cmd_help();
        } else if (cmd == "color") {
            cmd_color(args);
        } else if (cmd == "exit" || cmd == "quit" || cmd == "q") {
            save_notes();
            cout << GREEN << "\n  [OK] 数据已保存。再见！\n" << RESET;
            break;
        } else {
            cout << RED << "  [ERROR] 未知命令: " << RESET << BOLD << cmd << RESET
                 << "。输入 " << GREEN << "help" << RESET << " 查看可用命令。" << endl;
        }
    }
    return 0;
}