from flask import Flask, render_template, request, redirect, url_for, make_response, flash
import sqlite3
from markupsafe import escape
import os

UNSAFE = os.getenv('UNSAFE', False)

def init_db():
    db = sqlite3.connect('test.db')
    db.cursor().execute('DROP TABLE IF EXISTS sessions')
    db.cursor().execute('DROP TABLE IF EXISTS users')
    db.cursor().execute('DROP TABLE IF EXISTS comments')
    db.cursor().execute('CREATE TABLE IF NOT EXISTS comments '
                        '(id INTEGER PRIMARY KEY, '
                        'username TEXT,'
                        'comment TEXT)')
    db.cursor().execute('CREATE TABLE IF NOT EXISTS users '
                        '(id INTEGER PRIMARY KEY, '
                        'username TEXT,'
                        'password TEXT)')
    db.cursor().execute('CREATE TABLE IF NOT EXISTS sessions '
                        '(id INTEGER PRIMARY KEY, '
                        'username TEXT,'
                        'session_id TEXT,'
                        'csrf_token TEXT)')
    db.cursor().execute('INSERT INTO users (username, password) VALUES (?, ?)', ('zero', '123456'))
    db.commit()
    return db

init_db()

# 连接数据库
def connect_db():
    db = sqlite3.connect('test.db')
    return db

# 添加评论
def add_comment(comment, request):
    db = connect_db()

    # check session
    session_id = request.cookies.get('session')
    csrf_token = request.form.get('csrf_token')
    if session_id is None:
        user_token = None
    else:
        user_token = db.cursor().execute('SELECT username, csrf_token FROM sessions WHERE session_id=?', (session_id,)).fetchone()
    
    match = False
    username = None
    if user_token is not None:
        username, token = user_token
        if token == csrf_token:
            match = True

    if not UNSAFE:
        if username is not None and not match:
            return True

    db.cursor().execute('INSERT INTO comments (username, comment) VALUES (?, ?)', (username, comment))
    db.commit()
    return False
                                        

# 得到评论
def get_comments(search_query=None):
    db = connect_db()
    results = []
    get_all_query = 'SELECT username, comment FROM comments'
    for (username, comment,) in db.cursor().execute(get_all_query).fetchall():
        if search_query is None or search_query in comment:
            results.append((username, comment))
    return results


# 启动flask
app = Flask(__name__)
@app.route('/', methods=['GET', 'POST'])
def index():

    # CSRF Demo Site
    host = request.headers['Host']
    if host and 'localhost' in host:
        return render_template('csrf.html')
    
    # Site Index
    csrf_warning = False
    if request.method == 'POST':
        comment = request.form.get('comment')
        if comment:
            csrf_warning = add_comment(comment, request)

    search_query = request.args.get('q')

    session_id = request.cookies.get('session')
    db = connect_db()
    user_token = db.cursor().execute('SELECT username, csrf_token FROM sessions WHERE session_id=?', (session_id,)).fetchone()

    if user_token is None:
        user, token = None, None
    else:
        print(user_token)
        user, token = user_token

    message = request.cookies.get('message')

    if csrf_warning:
        message = "CSRF Token 不匹配!"
        
    comments = get_comments(search_query)

    if not UNSAFE:
        if search_query is not None:
            search_query = escape(search_query)

        comments = [
            [escape(comment), escape(username) if username is not None else None] 
            for comment, username in comments
        ]

    res = render_template('index.html',
                           comments=comments,
                           search_query=search_query,
                           user=user,
                           token=token,
                           message=message)
        
    res = make_response(res)
    res.set_cookie('message', "", expires=0)

    return res
        
@app.route('/login', methods=['GET'])
def login_page():
    return render_template('login.html')

@app.route('/login', methods=['POST'])
def login():
    username = request.form['username']
    password = request.form['password']
    
    db = connect_db()

    if UNSAFE:
        user = db.cursor().execute(f"SELECT username FROM users WHERE username='{username}' AND password='{password}'").fetchone()
        if user is not None:
            username = user[0]
    else:
        user = db.cursor().execute('SELECT username FROM users WHERE username=(?) AND password=(?)', (username, password)).fetchone()

    res = redirect(url_for('index'))
    if user is not None:
        session_id = os.urandom(16).hex()
        csrf_token = os.urandom(16).hex()
        db.cursor().execute('INSERT INTO sessions (username, session_id, csrf_token) VALUES (?, ?, ?)', (username, session_id, csrf_token))
        db.commit()

        res.set_cookie('session', session_id)
        res.set_cookie('message', "登陆成功")
    else:
        res.set_cookie('message', "登陆失败")
    return res

@app.route('/logout', methods=['POST', 'GET'])
def logout():
    res = redirect(url_for('index'))
    session_id = request.cookies.get('session')
    db = connect_db()
    db.cursor().execute('DELETE FROM sessions WHERE session_id=?', (session_id,))
    db.commit()

    res.set_cookie('session', '', expires=0)
    return res
