
def eval_text(typed_text, target_text):
    result = []

    # splits both strings into individual words using spaces as separators
    target_words = target_text.split(" ")
    typed_words = typed_text.split(" ")

    # gets the smaller list length to avoid an IndexError
    min_index = min(len(typed_words), len(target_words))

    for index in range(min_index):
        # accounts for the space before the current word since the inner loop evaluates its characters next
        if index > 0:
            result.append(True)

        # gets the target and typed words at the current index from the list
        typed_word = typed_words[index]
        target_word = target_words[index]

        # gets the shorter word length to avoid an IndexError
        min_length = min(len(typed_word), len(target_word))

        # compares each character within the current words
        for char in range(min_length):
            value = target_word[char] == typed_word[char]
            result.append(value)

        # appends False for any extra characters beyond the target word length
        for extra_char in range(len(typed_word) - len(target_word)):
            result.append(False)

    return result
