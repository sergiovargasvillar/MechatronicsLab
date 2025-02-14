import os
import logging
from logging.handlers import TimedRotatingFileHandler
from datetime import datetime

class CustomFormatter(logging.Formatter):
    def formatTime(self, record, datefmt=None):
        ct = self.converter(record.created)
        if datefmt:
            s = datetime.fromtimestamp(record.created).strftime(datefmt)
        else:
            t = datetime.fromtimestamp(record.created)
            s = t.strftime("%H:%M:%S,%f")[:-3]
        return s

def setup_logger():
    if not logging.getLogger().hasHandlers():
        # Create a logger
        logger = logging.getLogger()
        logger.setLevel(logging.INFO)

        # Create a file handler
        log_directory = 'logs'
        if not os.path.exists(log_directory):
            os.makedirs(log_directory)
        log_file = os.path.join(log_directory, f'{datetime.now().strftime("%Y-%m-%d_%H-%M-%S")}.log')
        file_handler = TimedRotatingFileHandler(log_file, when='midnight', interval=1)
        file_handler.suffix = '%Y-%m-%d.log'
        file_handler.setLevel(logging.INFO)

        # Create a stream handler
        stream_handler = logging.StreamHandler()
        stream_handler.setLevel(logging.INFO)

        # Create a custom formatter that includes milliseconds
        formatter = CustomFormatter('%(levelname)s - %(asctime)s - %(message)s', datefmt='%H:%M:%S,%f')
        file_handler.setFormatter(formatter)
        stream_handler.setFormatter(formatter)

        # Add both handlers to the logger
        logger.addHandler(file_handler)
        logger.addHandler(stream_handler)

# Call the setup_logger function to configure the logger
setup_logger()
