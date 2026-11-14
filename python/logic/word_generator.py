import random

temp_word_bank = {
    "1k": ["pour", "cycle", "silent", "greedy", "dirty", "governor", "throat", "horses", "elite", "spicy"],
    "5k": ["minister", "quill", "sneeze", "jump", "extend", "periodic", "park", "insect", "wait", "dreary"],
    "10k": ["minister", "quill", "sneeze", "jump", "extend", "periodic", "park", "insect", "wait", "dreary"],
    "25k": ["pizzas", "tangible", "psychotic", "cable", "legal", "manage", "uttermost", "sweltering", "dysfunctional", "attraction"]
}

def generate_words(difficulty, count=30):
    if (difficulty not in temp_word_bank):
        return
    else:
        return random.choices(temp_word_bank[difficulty], k=count)