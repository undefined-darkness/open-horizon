import logging
import os
from bottle import default_app, route, request, error
from datetime import datetime, timedelta

IP_LIST = dict()
LIFE_TIME = timedelta(minutes=30)
REFRESH_TIME = timedelta(minutes=1)
PREVIOUS_CHECK = datetime.utcnow()
LOG_PATH = os.path.join(os.path.dirname(__file__), 'IpLogger.log')
logging.basicConfig(
    filename=LOG_PATH, level=logging.INFO,
    format='%(asctime)s %(levelname)s %(message)s')
logging.info('Server started')
STR_APP = 'open-horizon'
STR_VERSION = '1'


def deco_version_check(f):
    def wrapper(*args, **kws):
        if not (
                request.GET.get('app', '') == STR_APP and
                request.GET.get('version', '') == STR_VERSION):
            return
        return f()
    return wrapper


def clear_old_records():
    global PREVIOUS_CHECK, IP_LIST
    ips = []
    if IP_LIST:
        t0 = datetime.utcnow()
        if t0 - PREVIOUS_CHECK > REFRESH_TIME:
            PREVIOUS_CHECK = t0
            for ip, t in IP_LIST.items():
                if t0 - t > LIFE_TIME:
                    logging.info('ip removed:{}'.format(IP_LIST.pop(ip, None)))
                else:
                    ips.append(ip)
        else:
            ips = IP_LIST.keys()
    return ips


@error(404)
def error_404(error):
    return 'Ya ain\'t allowed to be here'


@route('/get', method='GET')
@deco_version_check
def return_ip_list():
    ips = clear_old_records()
    logging.info('ip list request')
    return '\n'.join(ips)


@route('/log', method='GET')
@deco_version_check
def log_client_ip():
    global IP_LIST
    addr = request.environ.get('REMOTE_ADDR')
    IP_LIST[addr] = datetime.utcnow()
    logging.info('ip logged:{}'.format(addr))
    return '''ATTENTIONThis computer system has been seized
    by the United States Secret Service in the interests of
    National Security. Your IP has been logged.'''

application = default_app()
