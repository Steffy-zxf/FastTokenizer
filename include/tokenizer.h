// Copyright (c) 2021 PaddlePaddle Authors & liustung. All Rights Reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef INCLUDE_TOKENIZER_H_
#define INCLUDE_TOKENIZER_H_

#include <utf8proc.h>

#include <string>
#include <vector>
#include <unordered_map>

using std::wstring;
using std::string;
using std::shared_ptr;
using std::vector;
using std::unordered_map;
using std::size_t;


using Vocab = unordered_map<std::wstring, size_t>;
using InvVocab = unordered_map<size_t, wstring>;

class BasicTokenizer {
 public:
  explicit BasicTokenizer(bool do_lower_case = true);
  vector<wstring> Tokenize(const string& text) const;

 private:
  wstring clean_text(const wstring& text) const;
  bool is_chinese_char(const wchar_t& ch) const;
  wstring tokenize_chinese_chars(const wstring& text) const;
  wstring run_strip_accents(const wstring& text) const;
  vector<wstring> run_split_on_punc(const wstring& text) const;

  bool do_lower_case_{true};
};


class WordPieceTokenizer {
 public:
  explicit WordPieceTokenizer(
    const shared_ptr<Vocab>& vocab,
    const wstring& unk_token = L"[UNK]",
    const size_t max_input_chars_per_word = 100);
  vector<wstring> Tokenize(const wstring& text) const;

 private:
  shared_ptr<Vocab> vocab_;
  wstring unk_token_{L"[UNK]"};
  size_t max_input_chars_per_word_;
};


class FullTokenizer {
 public:
  explicit FullTokenizer(const string& vocab_file, bool do_lower_case = true);
  vector<wstring> Tokenize(const string& text) const;
  vector<size_t> ConvertTokensToIds(const vector<wstring>& text) const;

 private:
  shared_ptr<Vocab> vocab_;
  InvVocab inv_vocab_;
  string vocab_file_;
  bool do_lower_case_{true};
  BasicTokenizer basic_tokenizer_;
  WordPieceTokenizer word_piece_tokenizer_;
};


class BertTokenizer {
 public:
    explicit BertTokenizer(
      const string& vocab_file,
      bool do_lower_case = true,
      const wstring& unk_token = L"[UNK]",
      const wstring& pad_token = L"[UNK]",
      const wstring& cls_token = L"[UNK]",
      const wstring& mask_token = L"[MASK]",
      const wstring& sep_token = L"[SEP]",
      const string& padding_site = "right");

    vector<wstring> Tokenize(const string& text) const;
    vector<size_t> BuildInputsWithSpecialTokens(
      const vector<size_t>& token_ids_0,
      const vector<size_t>& token_ids_1 = vector<size_t>()) const;
    vector<size_t> CreateTokenTypeIdsFromSequences(
      const vector<size_t>& token_ids_0,
      const vector<size_t>& token_ids_1 = vector<size_t>()) const;
    vector<size_t> ConvertTokensToIds(
      const vector<wstring>& tokens) const;
    string ConvertTokensToString(
      const vector<wstring>& tokens) const;
    vector<wstring> ConvertIdsToTokens(
      const vector<size_t>& token_ids);
    unordered_map<string, vector<size_t>> TruncateSequence(
      vector<size_t>* ids,
      vector<size_t>* pair_ids,
      const int num_tokens_to_remove = 0,
      const string&  truncation_strategy = "longest_first",
      const size_t stride = 0) const;
    vector<size_t> GetSpecialTokensMask(
      const vector<size_t>& token_ids_0,
      const vector<size_t>& token_ids_1 = vector<size_t>(),
      const bool already_has_special_tokens = false) const;
    size_t GetNumSpecialTokensToAdd(const bool pair = false) const;
    unordered_map<string, vector<size_t>> Encode(
      const string& text,
      const string& text_pair = "",
      const int max_seq_len = -1,
      bool pad_to_max_seq_len = false,
      bool return_length = false,
      bool return_token_type_ids = true,
      bool return_position_ids = false,
      bool return_attention_mask = false,
      const string&  truncation_strategy = "longest_first",
      bool return_overflowing_tokens = false,
      bool return_special_tokens_mask = false) const;

 private:
    vector<size_t> get_input_ids(
      const string& text) const;
    vector<wstring> all_special_tokens_;
    vector<size_t> all_special_token_ids_;
    string vocab_file_;
    shared_ptr<Vocab> vocab_;
    InvVocab inv_vocab_;
    bool do_lower_case_{true};
    BasicTokenizer basic_tokenizer_;
    WordPieceTokenizer word_piece_tokenizer_;
    wstring unk_token_, cls_token_, mask_token_, pad_token_, sep_token_;
    size_t unk_token_id_, cls_token_id_,
      mask_token_id_, pad_token_id_, sep_token_id_;
    string padding_site_{"right"};
};

#endif  // INCLUDE_TOKENIZER_H_
