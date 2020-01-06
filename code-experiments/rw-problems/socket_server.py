"""
The socket server in Python.

Uses the toy_socket_evaluator to evaluate problems from the toy-socket suite. Additional evaluators
can be used -- whether they are included or not depends on the values of the respective variables
(see the variables that start with EVALUATE_). These definitions can be modified directly or through
do.py.

If the server receives the message 'SHUTDOWN', it shuts down.

Change code below to connect it to other evaluators (for other suites) -- see occurrences of
'ADD HERE'.
"""
import sys
import socket
from toy_socket.toy_socket_evaluator import evaluate_toy_socket

HOST = ''            # Symbolic name, meaning all available interfaces
MESSAGE_SIZE = 8000  # Should be large enough to contain a number of x-values
PRECISION_Y = 16     # Precision used to write objective values

EVALUATE_RW_MARIO_GAN = 0
if EVALUATE_RW_MARIO_GAN:
    from mario_gan.mario_gan_evaluator import evaluate_mario_gan
# ADD HERE imports from other evaluators, for example
# EVALUATE_MY_EVALUATOR = 0
# if EVALUATE_MY_EVALUATOR:
# from .my_suite.my_evaluator import evaluate_my_suite


def evaluate_message(message):
    """Parses the message and calls an evaluator to compute the evaluation. Then constructs a
    response. Returns the response."""
    try:
        # Parse the message
        msg = message.split(' ')
        suite_name = msg[msg.index('n') + 1]
        func = int(msg[msg.index('f') + 1])
        dimension = int(msg[msg.index('d') + 1])
        instance = int(msg[msg.index('i') + 1])
        num_objectives = int(msg[msg.index('o') + 1])
        x = [float(m) for m in msg[msg.index('x') + 1:]]
        if len(x) != dimension:
            raise ValueError('Number of x values {} does not match dimension {}'.format(len(x),
                                                                                        dimension))

        # Find the right evaluator
        evaluate = None
        if 'toy-socket' in suite_name:
            evaluate = evaluate_toy_socket
        elif EVALUATE_RW_MARIO_GAN > 0:
            if 'mario-gan' in suite_name or 'mario-gan-biobj' in suite_name:
                evaluate = evaluate_mario_gan
        # ADD HERE the function for another evaluator, for example
        # elif EVALUATE_MY_EVALUATOR > 0:
        #     if 'my-suite' in suite_name:
        #         evaluate = evaluate_my_suite
        if evaluate is None:
            raise ValueError('Suite {} not supported'.format(suite_name))
        # Evaluate x and save the result to y
        y = evaluate(suite_name, num_objectives, func, instance, x)
        # Construct the response
        response = ''
        for yi in y:
            response += '{:.{p}e} '.format(yi, p=PRECISION_Y)
        return str.encode(response)
    except Exception as e:
        print('Error within message evaluation: {}'.format(e))
        raise e


def socket_server_start(port, silent=False):
    s = None
    try:
        # Create socket
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Bind socket to local host and port
        try:
            s.bind((HOST, port))
        except socket.error as e:
            print('Bind failed: {}'.format(e))
            raise e

        # Start listening on socket
        s.listen(1)
        print('Socket server (Python) ready, listening on port {}'.format(port))

        # Talk with the client
        while True:
            try:
                # Wait to accept a connection - blocking call
                conn, addr = s.accept()
            except socket.error as e:
                print('Accept failed: {}'.format(e))
                raise e
            except KeyboardInterrupt or SystemExit:
                print('Server terminated')
                if s is not None:
                    s.close()
                return 0
            with conn:
                # Read the message
                message = conn.recv(MESSAGE_SIZE).decode("utf-8")
                # Make sure to remove and null endings
                message = message.split('\x00', 1)[0]
                if not silent:
                    print('Received message: {}'.format(message))
                # Check if the message is a request for shut down
                if message == 'SHUTDOWN':
                    print('Shutting down socket server (Python) ')
                    return
                # Parse the message and evaluate its contents using an evaluator
                response = evaluate_message(message)
                # Send the response
                conn.sendall(response)
                if not silent:
                    print('Sent response: {}'.format(response.decode("utf-8")))
    except Exception as e:
        print('Error: {}'.format(e))
        raise e
    finally:
        if s is not None:
            print('Closing socket')
            s.close()


if __name__ == '__main__':
    silent = False
    port = None
    if 2 <= len(sys.argv) <= 3:
        port = int(sys.argv[1])
        print('Socket server (Python) called on port {}'.format(port))
        if len(sys.argv) == 3:
            if sys.argv[2] == 'silent':
                silent = True
            else:
                print('Ignoring input option {}'.format(sys.argv[2]))
    else:
        print('Incorrect options\nUsage:\nsocket_server PORT <\"silent\">')
        exit(-1)
    socket_server_start(port=port, silent=silent)
