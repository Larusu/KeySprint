def calculate_extra_chars(typed_text, target_text):
    total_extra_chars = 0;

    target_words = target_text.split(" ")
    typed_words = typed_text.split(" ")
    min_index = min(len(typed_words), len(target_words))
    
    for index in range(min_index):

        # gets the target and typed words at the current index from the list
        typed_word = typed_words[index]
        target_word = target_words[index]

        if len(typed_word) > len(target_word):
            total_extra_chars += len(typed_word) - len(target_word)

    return total_extra_chars

def calculate_incorrect_chars(total_extra_chars, all_char_results):
    extra_chars = total_extra_chars
    char_results = all_char_results.count(False)

    total_incorrect_chars = char_results - extra_chars

    return total_incorrect_chars

def calculate_correct_chars(all_char_results):
    correct_chars = all_char_results.count(True)

    return correct_chars

def calculate_accuracy(total_extra_chars, total_incorrect_chars, total_correct_chars):
    if total_extra_chars + total_incorrect_chars + total_correct_chars == 0:
        return 0.0
        
    accuracy = total_correct_chars / (total_correct_chars + total_incorrect_chars + total_extra_chars) * 100

    return accuracy

def calculate_wpm(total_correct_chars, duration):
    if duration == 0:
        return 0.0

    total_wpm = (total_correct_chars / 5) / (duration / 60)

    return total_wpm