import logging
from logging.handlers import SMTPHandler, RotatingFileHandler
import os
# from elasticsearch import Elasticsearch
from flask import Flask
from flask_sqlalchemy import SQLAlchemy
from flask_migrate import Migrate
from flask_login import LoginManager
from flask_mail import Mail
from flask_bootstrap import Bootstrap
from flask_moment import Moment
from config import Config

app = Flask(__name__)
app.config.from_object(Config)
# app.elasticsearch = Elasticsearch([app.config['ELASTICSEARCH_URL']]) \
#         if app.config['ELASTICSEARCH_URL'] else None
db = SQLAlchemy(app)
migrate = Migrate(app, db)
login = LoginManager(app)
login.login_view = 'login'
mail = Mail(app)
bootstrap = Bootstrap(app)
moment = Moment(app)

if not app.debug:
    if app.config['MAIL_SERVER']:
        auth = None
        if app.config['MAIL_USERNAME'] or app.config['MAIL_PASSWORD']:
            auth = (app.config['MAIL_USERNAME'], app.config['MAIL_PASSWORD'])
        secure = None
        if app.config['MAIL_USE_TLS']:
            secure = ()
        mail_handler = SMTPHandler(
            mailhost=(app.config['MAIL_SERVER'], app.config['MAIL_PORT']),
            fromaddr='no-reply@' + app.config['MAIL_SERVER'],
            toaddrs=app.config['ADMINS'], subject='HMS Failure',
            credentials=auth, secure=secure)
        mail_handler.setLevel(logging.ERROR)
        app.logger.addHandler(mail_handler)

    if not os.path.exists('logs'):
        os.mkdir('logs')
    file_handler = RotatingFileHandler('logs/hms.log', maxBytes=10240,
                                       backupCount=10)
    file_handler.setFormatter(logging.Formatter(
        '%(asctime)s %(levelname)s: %(message)s [in %(pathname)s:%(lineno)d]'))
    file_handler.setLevel(logging.INFO)
    app.logger.addHandler(file_handler)

    app.logger.setLevel(logging.INFO)
    app.logger.info('HMS startup')

from app import routes, models, errors
from app.models import User, Movie, Order
db.create_all()
# Admin user is created on startup using config unless such a user already exists.
if(User.query.filter_by(username='admin').first() is None):
    admin = User(user_cat='manager',username='admin',email='admin@gmail.com')
    admin.password_hash = generate_password_hash(app.config['ADMIN_PASSWORD'])
    
    db.session.add(admin)
    db.session.commit()

