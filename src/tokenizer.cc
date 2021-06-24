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

#include <utf8proc.h>

#include <algorithm>
#include <locale>
#include <codecvt>
#include <iostream>
#include <fstream>
#include <numeric>
#include <string>
#include <vector>
#include <unordered_map>

#include <boost/algorithm/string.hpp>

#include "include/tokenizer.h"


using std::bad_cast;
using std::cerr;
using std::cin;
using std::codecvt_utf8;
using std::cout;
using std::endl;
using std::exception;
using std::ifstream;
using std::min;
using std::runtime_error;
using std::unordered_map;
using std::shared_ptr;
using std::size_t;
using std::string;
using std::vector;
using std::wcout;
using std::wstring;
using std::wstring_convert;


const wstring kStripChars = L" \t\n\r\v\f";


bool IsControl(const wchar_t& ch) {
  if (ch == L'\t' || ch == L'\n' || ch == L'\r') return false;
  auto cat = utf8proc_category(ch);
  if (cat == UTF8PROC_CATEGORY_CC || cat == UTF8PROC_CATEGORY_CF) return true;
  return false;
}

bool IsWhiteSpace(const wchar_t& ch) {
  if (ch == L' ' || ch == L'\t' || ch == L'\n' || ch == L'\r') return true;
  auto cat = utf8proc_category(ch);
  if (cat == UTF8PROC_CATEGORY_ZS) return true;
  return false;
}

bool IsPunctuation(const wchar_t& ch) {
  if ((ch >= 33 && ch <= 47) || (ch >= 58 && ch <= 64) ||
      (ch >= 91 && ch <= 96) || (ch >= 123 && ch <= 126))
    return true;
  auto cat = utf8proc_category(ch);
  if (cat == UTF8PROC_CATEGORY_PD || cat == UTF8PROC_CATEGORY_PS ||
      cat == UTF8PROC_CATEGORY_PE || cat == UTF8PROC_CATEGORY_PC ||
      cat == UTF8PROC_CATEGORY_PO  // sometimes ¶ belong SO
      || cat == UTF8PROC_CATEGORY_PI || cat == UTF8PROC_CATEGORY_PF)
    return true;
  return false;
}

string NormalizeNfd(const string& s) {
  string ret;
  char* result = reinterpret_cast<char*>(
    utf8proc_NFD(reinterpret_cast<const unsigned char*>(s.c_str())));
  if (result) {
    ret = string(result);
    free(result);
    result = nullptr;
  }

  return ret;
}

bool IsStripChar(const wchar_t& ch) {
  return kStripChars.find(ch) != wstring::npos;
}

wstring Strip(const wstring& text) {
  wstring ret = text;
  if (ret.empty()) return ret;
  size_t pos = 0;
  while (pos < ret.size() && IsStripChar(ret[pos])) pos++;
  if (pos != 0) ret = ret.substr(pos, ret.size() - pos);
  pos = ret.size() - 1;
  while (IsStripChar(ret[pos])) pos--;
  return ret.substr(0, pos + 1);
}

vector<wstring> Split(const wstring& text) {
  vector<wstring> result;
  boost::split(result, text, boost::is_any_of(kStripChars));
  return result;
}

vector<wstring> WhiteSpaceTokenize(const wstring& text) {
  wstring rtext = Strip(text);
  if (rtext.empty()) return vector<wstring>();
  return Split(text);
}

wstring ConvertToUnicode(const string& text) {
  size_t i = 0;
  wstring ret;
  while (i < text.size()) {
    wchar_t codepoint;
    utf8proc_ssize_t forward = utf8proc_iterate(
      reinterpret_cast<const utf8proc_uint8_t *>(&text[i]),
      text.size() - i,
      reinterpret_cast<utf8proc_int32_t *>(&codepoint));
    if (forward < 0) return L"";
    ret += codepoint;
    i += forward;
  }
  return ret;
}

wstring ConvertUtf8ToWstr(const string& src) {
  wstring_convert<codecvt_utf8<wchar_t>> converter;
  return converter.from_bytes(src);
}

string ConvertWstrToUtf8(const wstring& src) {
  wstring_convert<codecvt_utf8<wchar_t>> converter;
  return converter.to_bytes(src);
}


static std::wstring convertToUnicode(const std::string& text) {
    size_t i = 0;
    std::wstring ret;
    while (i < text.size()) {
        wchar_t codepoint;
        utf8proc_ssize_t forward = utf8proc_iterate(
          reinterpret_cast<const utf8proc_uint8_t *>(&text[i]),
          text.size() - i,
          reinterpret_cast<utf8proc_int32_t *>(&codepoint));
        if (forward < 0) return L"";
        ret += codepoint;
        i += forward;
    }
    return ret;
}

string ConvertFromUnicode(const wstring& text) {
  char dst[64];
  string ret;
  for (auto ch : text) {
    utf8proc_ssize_t num = utf8proc_encode_char(
      ch, reinterpret_cast<utf8proc_uint8_t*>(dst));
    if (num <= 0) return "";
    ret += string(dst, dst + num);
  }
  return ret;
}

wstring ToLower(const wstring& s) {
  wstring ret(s.size(), L' ');
  for (size_t i = 0; i < s.size(); i++) {
    ret[i] = utf8proc_tolower(s[i]);
  }
  return ret;
}

shared_ptr<Vocab> LoadVocab(const string& vocab_file) {
  shared_ptr<Vocab> vocab(new Vocab);
  ifstream ifs(vocab_file, ifstream::in);
  if (!ifs) {
    throw runtime_error(
      "Open the vocab file failly, please check the file " + vocab_file + ".");
  }

  string line;
  size_t index = 0;
  while (getline(ifs, line)) {
    wstring token = ConvertToUnicode(line);
    // The input line cann't be converted to unicode.
    // The drop it.
    if (token.empty()) continue;
    token = Strip(token);
    (*vocab)[token] = index;
    index++;
  }
  ifs.close();
  return vocab;
}


BasicTokenizer::BasicTokenizer(bool do_lower_case /* = true */) :
  do_lower_case_(do_lower_case) {}

wstring BasicTokenizer::clean_text(const wstring& text) const {
  wstring output;
  for (const wchar_t& cp : text) {
    if (cp == 0 || cp == 0xfffd || IsControl(cp)) continue;
    if (IsWhiteSpace(cp))
      output += L" ";
    else
      output += cp;
  }
  return output;
}

bool BasicTokenizer::is_chinese_char(const wchar_t& ch) const {
  if ((ch >= 0x4E00 && ch <= 0x9FFF) || (ch >= 0x3400 && ch <= 0x4DBF) ||
      (ch >= 0x20000 && ch <= 0x2A6DF) || (ch >= 0x2A700 && ch <= 0x2B73F) ||
      (ch >= 0x2B740 && ch <= 0x2B81F) || (ch >= 0x2B820 && ch <= 0x2CEAF) ||
      (ch >= 0xF900 && ch <= 0xFAFF) || (ch >= 0x2F800 && ch <= 0x2FA1F))
    return true;
  return false;
}

wstring BasicTokenizer::tokenize_chinese_chars(
  const wstring& text) const {
  wstring output;
  for (auto& ch : text) {
    if (is_chinese_char(ch)) {
      output.push_back(L' ');
      output.push_back(ch);
      output.push_back(L' ');
    } else {
      output.push_back(ch);
    }
  }
  return output;
}

wstring BasicTokenizer::run_strip_accents(
  const wstring& text) const {
  // Strips accents from a piece of text.
  wstring unicode_text;
  try {
    unicode_text = ConvertToUnicode(NormalizeNfd(ConvertFromUnicode(text)));
  }
  catch (bad_cast& e) {
    cerr << e.what() << endl;
    return L"";
  }

  wstring output;
  for (auto& ch : unicode_text) {
    auto cat = utf8proc_category(ch);
    if (cat == UTF8PROC_CATEGORY_MN) continue;
    output.push_back(ch);
  }
  return output;
}

vector<wstring> BasicTokenizer::run_split_on_punc(
  const wstring& text) const {
  size_t i = 0;
  bool start_new_word = true;
  vector<wstring> output;
  while (i < text.size()) {
    wchar_t ch = text[i];
    if (IsPunctuation(ch)) {
      output.push_back(wstring(&ch, 1));
      start_new_word = true;
    } else {
      if (start_new_word) output.push_back(wstring());
      start_new_word = false;
      output[output.size() - 1] += ch;
    }
    i++;
  }
  return output;
}

vector<wstring> BasicTokenizer::Tokenize(
  const string& text) const {
  cout << "BasicTokenizer::Tokenize " << text <<" ";
  wstring unicode_text = ConvertToUnicode(text);
  wcout << L"ConvertToUnicode " << unicode_text << endl;
  unicode_text = clean_text(unicode_text);

  unicode_text = tokenize_chinese_chars(unicode_text);

  const vector<wstring>& original_tokens = WhiteSpaceTokenize(unicode_text);
  vector<wstring> split_tokens;
  for (wstring token : original_tokens) {
    if (do_lower_case_) {
      token = ToLower(token);
      token = run_strip_accents(token);
    }
    const auto& tokens = run_split_on_punc(token);
    split_tokens.insert(split_tokens.end(), tokens.begin(), tokens.end());
  }
  return WhiteSpaceTokenize(boost::join(split_tokens, L" "));
}


WordPieceTokenizer::WordPieceTokenizer(
  const shared_ptr<Vocab>& vocab,
  const wstring& unk_token /* = L"[UNK]"*/,
  const size_t max_input_chars_per_word /* = 100 */) :
  vocab_(vocab),
  unk_token_(unk_token),
  max_input_chars_per_word_(max_input_chars_per_word) {}

vector<wstring> WordPieceTokenizer::Tokenize(
  const wstring& text) const {
  vector<wstring> output_tokens;
  for (auto& token : WhiteSpaceTokenize(text)) {
    if (token.size() > max_input_chars_per_word_) {
      output_tokens.push_back(unk_token_);
    }
    bool is_bad = false;
    size_t start = 0;
    vector<wstring> sub_tokens;
    while (start < token.size()) {
      size_t end = token.size();
      wstring cur_sub_str;
      bool has_cur_sub_str = false;
      while (start < end) {
        wstring substr = token.substr(start, end - start);
        if (start > 0) substr = L"##" + substr;
        if (vocab_->find(substr) != vocab_->end()) {
          cur_sub_str = substr;
          has_cur_sub_str = true;
          break;
        }
        end--;
      }
      if (!has_cur_sub_str) {
        is_bad = true;
        break;
      }
      sub_tokens.push_back(cur_sub_str);
      start = end;
    }
    if (is_bad)
      output_tokens.push_back(unk_token_);
    else
      output_tokens.insert(output_tokens.end(), sub_tokens.begin(),
                          sub_tokens.end());
  }
  return output_tokens;
}

FullTokenizer::FullTokenizer(const string& vocab_file, bool do_lower_case):
  vocab_(LoadVocab(vocab_file)),
  basic_tokenizer_(BasicTokenizer(do_lower_case)),
  word_piece_tokenizer_(WordPieceTokenizer(vocab_)) {
    for (auto& v : *vocab_) inv_vocab_[v.second] = v.first;
}

vector<wstring> FullTokenizer::Tokenize(const string& text) const {
  vector<wstring> split_tokens;
  for (auto& token : basic_tokenizer_.Tokenize(text))
    for (auto& sub_token : word_piece_tokenizer_.Tokenize(token))
      split_tokens.push_back(sub_token);
  return split_tokens;
}

vector<size_t> FullTokenizer::ConvertTokensToIds(
    const vector<wstring>& text) const {
  vector<size_t> ret(text.size());
  for (size_t i = 0; i < text.size(); i++) {
    ret[i] = (*vocab_)[text[i]];
  }
  return ret;
}

BertTokenizer::BertTokenizer(
  const string& vocab_file,
  bool do_lower_case /* = true */,
  const wstring& unk_token /* = L"[UNK]" */,
  const wstring& pad_token /* = L"[PAD]" */,
  const wstring& cls_token /* = L"[CLS]" */,
  const wstring& mask_token /* = L"[MASK]" */,
  const wstring& sep_token /* = L"[SEP]" */,
  const string& padding_site /* = "right" */) :
  do_lower_case_(do_lower_case),
  vocab_file_(vocab_file),
  // vocab_: the map token_str to token_id
  vocab_(LoadVocab(vocab_file)),
  unk_token_(unk_token),
  pad_token_(pad_token),
  cls_token_(cls_token),
  mask_token_(mask_token),
  sep_token_(sep_token),
  padding_site_(padding_site),
  basic_tokenizer_(BasicTokenizer(do_lower_case_)),
  word_piece_tokenizer_(vocab_, unk_token) {
    unk_token_id_ = (*vocab_)[unk_token_];
    pad_token_id_ = (*vocab_)[pad_token_];
    cls_token_id_ = (*vocab_)[cls_token_];
    mask_token_id_ = (*vocab_)[mask_token_];
    sep_token_id_ = (*vocab_)[sep_token_];

    all_special_tokens_ = vector<wstring>(
      {unk_token_, pad_token_, cls_token_, mask_token_, sep_token_});
    all_special_token_ids_ = vector<size_t>(
      {unk_token_id_,
      pad_token_id_,
      cls_token_id_,
      mask_token_id_,
      sep_token_id_});

    // inv_vocab_: the map token_id to token_str
    for (auto& v : *vocab_) inv_vocab_[v.second] = v.first;
  }

vector<size_t> BertTokenizer::ConvertTokensToIds(
  const vector<wstring>& tokens) const {
    vector<size_t> token_ids(tokens.size());
    for (size_t i = 0; i < token_ids.size(); ++i) {
      auto iter = (*vocab_).find(tokens[i]);
      if (iter != (*vocab_).end()) {
        token_ids[i] = iter->second;
      } else {
        token_ids[i] = unk_token_id_;
      }
    }
    return token_ids;
  }

vector<wstring> BertTokenizer::ConvertIdsToTokens(
  const vector<size_t>& token_ids) {
    vector<wstring> text(token_ids.size());
    for (size_t i = 0; i < token_ids.size(); ++i) {
      const size_t id = token_ids[i];
      text[i] = inv_vocab_[id];
    }
    return text;
  }

string BertTokenizer::ConvertTokensToString(
  const vector<wstring>& tokens) const {
    string text = ConvertFromUnicode(boost::join(tokens, L" "));
    return text;
  }

vector<wstring> BertTokenizer::Tokenize(
  const string& text) const {
    vector<wstring> split_tokens;
    for (auto& token : basic_tokenizer_.Tokenize(text))
      for (auto& sub_token : word_piece_tokenizer_.Tokenize(token))
        split_tokens.push_back(sub_token);
    return split_tokens;
  }


vector<size_t> BertTokenizer::BuildInputsWithSpecialTokens(
  const vector<size_t>& token_ids_0,
  const vector<size_t>& token_ids_1) const {
    if (token_ids_1.size() == 0) {
      vector<size_t> inputs(2+token_ids_0.size());
      inputs.push_back(cls_token_id_);
      for (auto& token_id : token_ids_0) {
        inputs.push_back(token_id);
      }
      inputs.push_back(sep_token_id_);
      return inputs;
    } else {
      vector<size_t> inputs(3+token_ids_0.size()+token_ids_1.size());
      inputs.push_back(cls_token_id_);
      for (auto& token_id : token_ids_0) {
        inputs.push_back(token_id);
      }
      inputs.push_back(sep_token_id_);
      for (auto& token_id : token_ids_1) {
        inputs.push_back(token_id);
      }
      inputs.push_back(sep_token_id_);
      return inputs;
    }
  }

size_t BertTokenizer::GetNumSpecialTokensToAdd(const bool pair) const {
    if (pair) {
      return 3;
    } else {
      return 2;
    }
  }

vector<size_t> BertTokenizer::CreateTokenTypeIdsFromSequences(
  const vector<size_t>& token_ids_0,
  const vector<size_t>& token_ids_1 /* = vector<size_t>() */) const {
    if (token_ids_1.size() == 0) {
      vector<size_t> token_type_ids(token_ids_0.size()+2, 0);
      return token_type_ids;
    } else {
      vector<size_t> token_type_ids(
        token_ids_0.size()+ token_ids_1.size()+3);
      for (size_t i = 0; i <= token_ids_0.size(); i++) {
        token_type_ids[i] = 0;
      }
      for (size_t i = token_ids_0.size()+1 ; i <= token_type_ids.size(); i++) {
        token_type_ids[i] = 1;
      }
      return token_type_ids;
    }
  }

unordered_map<string, vector<size_t>> BertTokenizer::TruncateSequence(
  vector<size_t>* ids,
  vector<size_t>* pair_ids,
  const int num_tokens_to_remove /* = 0 */,
  const string&  truncation_strategy /* = "longest_first" */,
  const size_t stride /* = 0 */) const {
    unordered_map<string, vector<size_t>> res;
    vector<size_t> overflowing_token_ids = vector<size_t>();
    if (num_tokens_to_remove <= 0) {
      res["ids"] = (*ids);
      res["pair_ids"] = (*pair_ids);
      res["overflowing_token_ids"] = overflowing_token_ids;
      return res;
    }

    size_t window_len;
    if (truncation_strategy == "longest_first") {
      for (size_t i =0; i < num_tokens_to_remove; i++) {
        if ((pair_ids->size() == 0) || (ids->size() > pair_ids->size())) {
          if (overflowing_token_ids.size() == 0) {
            window_len = min(ids->size(), stride + 1);
          } else {
            window_len = 1;
          }

          for (size_t i = ids->size()-1;
            i > ids->size() - window_len - 1;
            i--) {
            overflowing_token_ids.push_back((*ids)[i]);
          }
          ids->pop_back();
        } else {
          if (overflowing_token_ids.size() == 0) {
            window_len = min(pair_ids->size(), stride+1);
          } else {
            window_len = 1;
          }
          for (size_t i = pair_ids->size()-1;
          i > pair_ids->size() - window_len - 1;
          i--) {
            overflowing_token_ids.push_back((*pair_ids)[i]);
          }
          pair_ids->pop_back();
        }
      }
      reverse(overflowing_token_ids.begin(), overflowing_token_ids.end());
    } else if (truncation_strategy == "only_first") {
      if (ids->size() > num_tokens_to_remove) {
        window_len = min(ids->size(), stride + num_tokens_to_remove);
        for (size_t i = ids->size()-1; i > ids->size() - window_len - 1; i--) {
          overflowing_token_ids.push_back((*ids)[i]);
        }
        for (size_t i = 0; i < num_tokens_to_remove; i++) {
          ids->pop_back();
        }
      } else {
        cerr << "We need to remove {num_tokens_to_remove} "
          "to truncate the input but the first sequence has a length "
          << ids->size()
          << ". Please select another truncation strategy than "
          << truncation_strategy
          <<", for instance \'longest_first\' or \'only_second\'."
          << endl;
      }
    } else if (
      truncation_strategy == "only_second" && pair_ids->size()== 0) {
        if (pair_ids->size() > num_tokens_to_remove) {
          window_len = min(pair_ids->size(), stride + num_tokens_to_remove);
          for (size_t i = pair_ids->size()-1;
          i > pair_ids->size() - window_len - 1;
          i--) {
            overflowing_token_ids.push_back((*pair_ids)[i]);
          }
          for (size_t i = 0; i < num_tokens_to_remove; i++) {
            pair_ids->pop_back();
          }
        } else {
          cerr << "We need to remove " << num_tokens_to_remove
            << " to truncate the input but the first sequence has a length "
            << ids->size()
            << ". Please select another truncation strategy than "
            << truncation_strategy
            << ", for instance \'longest_first\' or \'only_first\'."
            << endl;
        }
      }
    res["ids"] = (*ids);
    res["pair_ids"] = (*pair_ids);
    res["overflowing_token_ids"] = overflowing_token_ids;
    return res;
  }

vector<size_t> BertTokenizer::GetSpecialTokensMask(
  const vector<size_t>& token_ids_0,
  const vector<size_t>& token_ids_1 /* = vector<size_t>() */,
  const bool already_has_special_tokens /* = false */) const {
    if (already_has_special_tokens) {
      if (token_ids_1.size() != 0) {
        throw runtime_error(
          "You should not supply a second sequence if the provided sequence of "
          "ids is already formatted with special tokens for the model.");
      }
      vector<size_t> res = vector<size_t>(token_ids_0.size());
      for (size_t i = 0; i < res.size(); i++) {
        auto&& iter = find(all_special_token_ids_.begin(),
                          all_special_token_ids_.end(),
                          token_ids_0[i]);
        if (iter != all_special_token_ids_.end()) {
          res[i] = 1;
        } else {
          res[i] = 0;
        }
      }
      return res;
    }

    if (token_ids_1.size() != 0) {
      vector<size_t> res = vector<size_t>(
        3+token_ids_0.size()+token_ids_1.size(), 0);
      res[0] = 1;
      res[token_ids_0.size()+1] = 1;
      res[2+token_ids_0.size()+token_ids_1.size()] = 1;
      return res;
    } else {
      vector<size_t> res = vector<size_t>(2+token_ids_0.size(), 0);
      res[0] = 1;
      res[token_ids_0.size()+1] = 1;
      return res;
    }
  }

vector<size_t> BertTokenizer::get_input_ids(const string& text) const {
  vector<wstring> tokens = Tokenize(text);
  cout<< "text: "<< text << endl;
  for (auto v : tokens) {
    wcout << v << " ";
  }
  wcout << endl;
  vector<size_t> token_ids = ConvertTokensToIds(tokens);
  return token_ids;
}

unordered_map<string, vector<size_t>> BertTokenizer::Encode(
  const string& text,
  const string& text_pair /* = "" */,
  const int max_seq_len /* = -1 */,
  bool pad_to_max_seq_len /* = false */,
  bool return_length /* = false */,
  bool return_token_type_ids /* = true */,
  bool return_position_ids /* = false */,
  bool return_attention_mask /* = false */,
  const string&  truncation_strategy /* = "longest_first" */,
  bool return_overflowing_tokens /* = false */,
  bool return_special_tokens_mask /* = false */) const {
    cout << "check ids"<< endl;
    vector<size_t> ids = get_input_ids(text);
    vector<size_t> pair_ids = vector<size_t>();
    if (text_pair != "") {
      vector<size_t> res = get_input_ids(text_pair);
      pair_ids.resize(res.size());
      for (size_t i = 0; i < res.size(); i++) {
        pair_ids[i] = res[i];
      }
    }

    bool pair = false;
    if (pair_ids.size() != 0) {
      pair = true;
    }

    size_t len_ids = ids.size();
    size_t len_pair_ids = pair_ids.size();
    unordered_map<string, vector<size_t>> encoded_inputs;

    // Truncation: Handle max sequence length
    // If max_seq_len <= 0, then do nothing and keep the real length.
    // If max_seq_len > 0 and
    // all the input sequence len is over the max_seq_len,
    // then we truncate it.
    size_t total_len = len_ids + len_pair_ids + GetNumSpecialTokensToAdd(pair);
    if (max_seq_len > 0  && total_len > max_seq_len) {
      auto&& res = TruncateSequence(
        &ids, &pair_ids, total_len - max_seq_len, truncation_strategy);
      if (res.find("overflowing_token_ids") != res.end()) {
        encoded_inputs["overflowing_token_ids"] = res["overflowing_token_ids"];
        encoded_inputs["num_truncated_tokens"] = vector<size_t>(
          1, total_len - max_seq_len);
      }
    }

    // Add special tokens
    auto&& sequence = BuildInputsWithSpecialTokens(ids, pair_ids);
    auto&& token_type_ids = CreateTokenTypeIdsFromSequences(ids, pair_ids);

    // Build output dictionnary
    encoded_inputs["input_ids"] = sequence;
    if (return_token_type_ids) {
      encoded_inputs["token_type_ids"] = token_type_ids;
    }
    if (return_special_tokens_mask) {
      encoded_inputs["special_tokens_mask"] = GetSpecialTokensMask(
        ids, pair_ids);
    }
    if (return_length) {
      encoded_inputs["seq_len"] = vector<size_t>(
        1, encoded_inputs["input_ids"].size());
    }

    // Check lengths
    if (max_seq_len > 0 &&
    encoded_inputs["input_ids"].size() > max_seq_len ) {
      cout<< "check check check "
        << max_seq_len
        << " " << encoded_inputs["input_ids"].size()
        << endl;
      throw runtime_error(
        "There is something wrong with the input sequence length."
        " Please check it.");
    }

    // Padding
    bool needs_to_be_padded = false;
    if (pad_to_max_seq_len && max_seq_len > 0
      && (encoded_inputs["input_ids"].size() < max_seq_len)) {
        needs_to_be_padded = true;
    }

    if (needs_to_be_padded) {
      size_t difference = max_seq_len - encoded_inputs["input_ids"].size();
      if (padding_site_ == "right") {
        if (return_attention_mask) {
          vector<size_t> attention_mask(max_seq_len, 0);
          for (size_t i = 0; i < encoded_inputs["input_ids"].size(); i++) {
            attention_mask[i] = 1;
          }
          encoded_inputs["attention_mask"] = attention_mask;
        }

        if (return_token_type_ids) {
          encoded_inputs["token_type_ids"].resize(max_seq_len);
          for (size_t i =max_seq_len-1;
          i > (max_seq_len - 1 - difference); i--) {
            encoded_inputs["token_type_ids"][i] = pad_token_id_;
          }
        }

        if (return_special_tokens_mask) {
          encoded_inputs["special_tokens_mask"].resize(max_seq_len);
          for (size_t i = max_seq_len-1;
          i > (max_seq_len-1-difference); i--) {
            encoded_inputs["special_tokens_mask"][i] = 1;
          }
        }

        encoded_inputs["input_ids"].resize(max_seq_len);
        for (size_t i = max_seq_len-1;
        i> (max_seq_len-1-difference); i--) {
          encoded_inputs["input_ids"][i] = pad_token_id_;
        }
      } else if (padding_site_ == "left") {
        if (return_attention_mask) {
          vector<size_t> attention_mask = vector<size_t>(max_seq_len, 0);
          for (size_t i = difference; i < max_seq_len; i++) {
            attention_mask[i] = 1;
          }
        }

        if (return_token_type_ids) {
          vector<size_t> tmp(max_seq_len, pad_token_id_);
          for (size_t i = difference; i< max_seq_len; i++) {
            tmp[i] = encoded_inputs["token_type_ids"][i-difference];
          }
          encoded_inputs["token_type_ids"] = tmp;
        }

        if (return_special_tokens_mask) {
          vector<size_t> tmp(max_seq_len, 1);
          for (size_t i = difference; i < max_seq_len; i++) {
            tmp[i] = encoded_inputs["special_tokens_mask"][i-difference];
          }
          encoded_inputs["special_tokens_mask"] = tmp;
        }

        vector<size_t> tmp(max_seq_len, pad_token_id_);
        for (size_t i = difference; i < max_seq_len; i++) {
          tmp[i] = encoded_inputs["input_ids"][i-difference];
        }
        encoded_inputs["input_ids"] = tmp;
      }
    } else {
      if (return_attention_mask) {
        encoded_inputs["attention_mask"] = vector<size_t>(
          encoded_inputs["input_ids"].size(), 1);
      }
    }

    if (return_position_ids) {
      encoded_inputs["position_ids"] = vector<size_t>(
        encoded_inputs["input_ids"].size());
      for (size_t i = 0; i < encoded_inputs["input_ids"].size() ; i++) {
        encoded_inputs["position_ids"][i] = i;
      }
    }

    return encoded_inputs;
  }


int main() {
  BertTokenizer* tokenizer_ptr = nullptr;
  FullTokenizer* full_t_ptr = nullptr;
  try {
    tokenizer_ptr = new BertTokenizer("bert-base-chinese-vocab.txt");
  }
  catch (exception& e) {
    cerr << e.what() << endl;
    return -1;
  }

  try {
    full_t_ptr = new FullTokenizer("bert-base-chinese-vocab.txt");
  }
  catch (exception& e) {
    cerr << e.what() << endl;
    return -1;
  }

  string line;
  while (getline(cin, line)) {
    cout << "input " << line << endl;
    wstring t = ConvertUtf8ToWstr(line);
    wcout << L"ConvertUtf8ToWstr " << t << endl << endl;
    cout << "ConvertWstrToUtf8 " << ConvertWstrToUtf8(t) << endl;

    vector<wstring> tokens = full_t_ptr->Tokenize(line);
    vector<size_t> ids = full_t_ptr->ConvertTokensToIds(tokens);
    cout << "123 "<< line << endl;

    wcout << boost::join(tokens, L" ");
    cout << ConvertFromUnicode(boost::join(tokens, L" "))
      << "#"
      << "\t";
    for (size_t i = 0; i < ids.size(); i++) {
      if (i != 0) cout << " ";
      cout << ids[i];
    }
    cout << endl;

    unordered_map<string, vector<size_t>> res = tokenizer_ptr->Encode(
      line, "", 10, false, false, true, false);
    for (auto i : res) {
      cout << i.first << ":" <<endl;
      for (auto j : i.second) {
        cout << j << " ";
      }
      cout << endl;
    }

    unordered_map<string, vector<size_t>> res_2 = tokenizer_ptr->Encode(
      line, "这是个测试样例", 30, false, false, true, false);
    for (auto i : res_2) {
      cout << i.first << ":" << endl;
      for (auto j : i.second) {
        cout << j << " ";
      }
      cout << endl;
    }
  }

  return 0;
}
