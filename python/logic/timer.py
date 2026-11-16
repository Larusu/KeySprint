class Timer():
    def __init__(self, timer):
        self.duration = timer
        self.remaining_time = timer
        self.is_running = False
        self.is_finished = False
        self.elapsed_time = 0

    def start_timer(self):
        if self.is_running == False and self.is_finished == False:
            self.is_running = True

    def tick(self):
        if self.is_running == True:
            self.remaining_time -= 1

            if self.remaining_time <= 0:
                self.is_running = False
                self.is_finished = True
                self.remaining_time = 0

    def reset_timer(self, timer):
        self.duration = timer
        self.remaining_time = timer
        self.is_running = False
        self.is_finished = False
        self.elapsed_time = 0