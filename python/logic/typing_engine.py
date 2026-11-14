def eval_text(typed_text, target_text):
    result = []

    min_length = min(len(target_text), len(typed_text))

    for char in range(min_length):
        value = target_text[char] == typed_text[char]
        result.append(value)

    for extra_char in range(len(typed_text) - len(target_text)):
        result.append(False)
    
    return result

## test
print(eval_text("gandi", "ganda"))
print(eval_text("gandako", "ganda"))