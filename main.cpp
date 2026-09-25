#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <ctime>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

struct Note {
    string time_str;
    string content;
};

vector<Note> notes;
string current_file = "note.txt";

// 扫描当前目录，获取全部 .txt 笔记本文件名
vector<string> scan_all_notebooks() {
    vector<string> res;
    try {
        for (auto &entry: fs::directory_iterator(fs::current_path())) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                string name = entry.path().filename().string();
                if (name.substr(0, 6) != "backup" && name != "CMakeLists.txt") {
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

// 加载当前笔记本
void load_notes() {
    ifstream fin(current_file);
    if (!fin.is_open()) {
        return;
    }
    string time_buf, content_buf;
    while (getline(fin, time_buf) && getline(fin, content_buf)) {
        Note n;
        n.time_str = time_buf;
        n.content = content_buf;
        notes.push_back(n);
    }
    fin.close();
}

// 保存到当前笔记本
void save_notes() {
    ofstream fout(current_file);
    for (auto &item: notes) {
        fout << item.time_str << endl;
        fout << item.content << endl;
    }
    fout.close();
}

// 根据笔记本文件名，读取该笔记本全部笔记
vector<Note> read_notebook(const string &filename) {
    vector<Note> tmp_notes;
    ifstream fin(filename);
    if (!fin.is_open())
        return tmp_notes;
    string time_buf, content_buf;
    while (getline(fin, time_buf) && getline(fin, content_buf)) {
        Note n;
        n.time_str = time_buf;
        n.content = content_buf;
        tmp_notes.push_back(n);
    }
    fin.close();
    return tmp_notes;
}

// 【新】批量导出笔记本，自动生成 backup_xxx.txt
void export_selected_notebooks() {
    vector<string> book_list = scan_all_notebooks();
    if (book_list.empty()) {
        cout << "❌没有找到任何笔记本可以导出！" << endl;
        return;
    }
    cout << "\n==== 选择要导出的笔记本 ====" << endl;
    for (size_t i = 0; i < book_list.size(); i++) {
        cout << i + 1 << ". " << book_list[i] << endl;
    }
    cout << "请输入序号，多个序号用空格分开（例如：1 3）：";
    string line;
    getline(cin, line);

    // 解析输入的多个序号
    vector<int> selected_idx;
    string num;
    for (char ch: line) {
        if (ch == ' ') {
            if (!num.empty()) {
                selected_idx.push_back(stoi(num));
                num.clear();
            }
        } else {
            num += ch;
        }
    }
    if (!num.empty()) {
        selected_idx.push_back(stoi(num));
    }

    // 逐个导出选中笔记本
    for (int idx: selected_idx) {
        if (idx < 1 || (size_t) idx > book_list.size()) {
            cout << "⚠️序号" << idx << "无效，跳过" << endl;
            continue;
        }
        string book_name = book_list[idx - 1];
        vector<Note> book_data = read_notebook(book_name);
        string backup_name = "backup_" + book_name;
        ofstream fout(backup_name);
        fout << "====笔记本文件：" << book_name << "====" << endl;
        for (auto &item: book_data) {
            fout << item.time_str << endl;
            fout << item.content << endl;
        }
        fout.close();
        cout << "✅已导出：" << book_name << " -> " << backup_name << endl;
    }
}

// 导入备份文件，追加到当前笔记本
void import_backup() {
    string import_filename;
    cout << "请输入同目录下备份文件名（例如 backup_note.txt）：";
    getline(cin, import_filename);
    ifstream fin(import_filename);
    if (!fin.is_open()) {
        cout << "❌无法打开文件，请确认文件放在程序同一文件夹！" << endl;
        return;
    }
    string line;
    getline(fin, line);
    string time_buf, content_buf;
    int import_count = 0;
    while (getline(fin, time_buf) && getline(fin, content_buf)) {
        Note n;
        n.time_str = time_buf;
        n.content = content_buf;
        notes.push_back(n);
        import_count++;
    }
    fin.close();
    save_notes();
    cout << "✅导入完成！一共导入 " << import_count << " 条笔记，已追加进当前笔记本" << endl;
}

// 新建自定义命名笔记本
void create_new_notebook() {
    string new_name;
    cout << "输入新笔记本文件名（直接写 xxx）：";
    getline(cin, new_name);
    new_name += ".txt";
    save_notes();
    notes.clear();
    current_file = new_name;
    ofstream create_file(current_file);
    create_file.close();
    load_notes();
    cout << "✅成功新建并切换到笔记本：" << current_file << endl;
}

// 打印帮助信息
void show_help() {
    cout << "\n===== 帮助说明 =====" << endl;
    cout << "1 添加笔记：录入一条新笔记，自动记录创建时间" << endl;
    cout << "2 查看全部笔记：列出当前笔记本所有笔记" << endl;
    cout << "3 删除指定笔记：输入编号删除单条笔记" << endl;
    cout << "4 修改指定笔记：修改笔记内容，不改动原始创建时间" << endl;
    cout << "5 关键词搜索笔记：按内容检索笔记" << endl;
    cout << "6 清空当前笔记本：全部删除，需要输入y确认" << endl;
    cout << "7 切换笔记本：列出全部笔记本（包含你新建的）" << endl;
    cout << "8 批量导出笔记本：选择1个或多个笔记本导出，生成 backup_xxx.txt" << endl;
    cout << "9 查看帮助说明" << endl;
    cout << "10 新建自定义笔记本：自己输入文件名创建笔记本" << endl;
    cout << "11 导入备份文件：读取备份文件，追加笔记到当前笔记本" << endl;
    cout << "0 退出程序：保存数据并退出" << endl;
    cout << "\n⚠️重要提醒：必须选0正常退出保存。直接点窗口关闭按钮会丢失数据！" << endl;
}

int main() {
    if (!fs::exists("note.txt")) {
        ofstream f("note.txt");
        f.close();
    }
    load_notes();
    int op;
    while (true) {
        cout << "\n====简易记事本 V1.5====" << endl;
        cout << "当前笔记本文件：" << current_file << endl;
        cout << "1 添加笔记" << endl;
        cout << "2 查看全部笔记" << endl;
        cout << "3 删除指定笔记" << endl;
        cout << "4 修改指定笔记" << endl;
        cout << "5 关键词搜索笔记" << endl;
        cout << "6 清空当前笔记本全部笔记" << endl;
        cout << "7 切换笔记本" << endl;
        cout << "8 批量导出笔记本" << endl;
        cout << "9 查看帮助说明" << endl;
        cout << "10 新建自定义笔记本" << endl;
        cout << "11 导入备份文件" << endl;
        cout << "0 退出程序" << endl;
        cout << "请输入选择：";
        cin >> op;
        cin.ignore(1000, '\n');
        if (op == 1) {
            string content;
            cout << "输入笔记内容：";
            getline(cin, content);
            Note n;
            n.time_str = get_now_time();
            n.content = content;
            notes.push_back(n);
            save_notes();
            cout << "✅保存成功，记录时间：" << n.time_str << endl;
        } else if (op == 2) {
            cout << "\n-----笔记列表-----" << endl;
            for (size_t i = 0; i < notes.size(); i++) {
                cout << i + 1 << ". [" << notes[i].time_str << "] " << notes[i].content << endl;
            }
        } else if (op == 3) {
            int idx;
            cout << "输入要删除的笔记编号：";
            cin >> idx;
            cin.ignore(1000, '\n');
            if (idx < 1 || (size_t) idx > notes.size()) {
                cout << "❌编号无效！" << endl;
            } else {
                notes.erase(notes.begin() + idx - 1);
                save_notes();
                cout << "✅删除完成" << endl;
            }
        } else if (op == 4) {
            int idx;
            cout << "输入要修改的笔记编号：";
            cin >> idx;
            cin.ignore(1000, '\n');
            if (idx < 1 || (size_t) idx > notes.size()) {
                cout << "❌编号无效！" << endl;
            } else {
                string new_text;
                cout << "输入新笔记内容：";
                getline(cin, new_text);
                notes[idx - 1].content = new_text;
                save_notes();
                cout << "✅修改完成（原始创建时间不变）" << endl;
            }
        } else if (op == 5) {
            string key;
            cout << "输入搜索关键词：";
            getline(cin, key);
            cout << "\n---搜索结果---" << endl;
            for (auto &item: notes) {
                if (item.content.find(key) != string::npos) {
                    cout << "[" << item.time_str << "] " << item.content << endl;
                }
            }
        } else if (op == 6) {
            string confirm;
            cout << "确认清空当前笔记本全部笔记？输入 y 确认，其他字符取消：";
            getline(cin, confirm);
            if (confirm == "y" || confirm == "Y") {
                notes.clear();
                save_notes();
                cout << "✅已清空当前笔记本" << endl;
            } else {
                cout << "已取消清空操作" << endl;
            }
        } else if (op == 7) {
            cout << "\n====切换笔记本====" << endl;
            vector<string> book_list = scan_all_notebooks();
            if (book_list.empty()) {
                cout << "没有找到任何笔记本！" << endl;
                continue;
            }
            for (size_t i = 0; i < book_list.size(); i++) {
                cout << i + 1 << ". " << book_list[i] << endl;
            }
            int select;
            cout << "请输入序号选择笔记本：";
            cin >> select;
            cin.ignore(1000, '\n');

            if (select < 1 || (size_t) select > book_list.size()) {
                cout << "❌无效选择，不切换笔记本" << endl;
                continue;
            }
            save_notes();
            notes.clear();
            current_file = book_list[select - 1];
            load_notes();
            cout << "✅已切换到：" << current_file << endl;
        } else if (op == 8) {
            export_selected_notebooks();
        } else if (op == 9) {
            show_help();
        } else if (op == 10) {
            create_new_notebook();
        } else if (op == 11) {
            import_backup();
        } else if (op == 0) {
            save_notes();
            cout << "✅数据已保存，程序退出。" << endl;
            break;
        } else {
            cout << "❌无效选项，请重新输入！" << endl;
        }
    }
    return 0;
}