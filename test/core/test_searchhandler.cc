/*GRB*
  Gerbera - https://gerbera.io/

  test_searchhandler.cc - this file is part of Gerbera.

  Copyright (C) 2018-2021 Gerbera Contributors

  Gerbera is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2
  as published by the Free Software Foundation.

  Gerbera is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.

  $Id$
*/

#include <gtest/gtest.h>
#include <regex>

#include "database/search_handler.h"

using upVecUpST = std::unique_ptr<std::vector<std::unique_ptr<SearchToken>>>;
static decltype(auto) getAllTokens(const std::string& input)
{
    SearchLexer lexer { input };
    upVecUpST searchTokens = std::make_unique<std::vector<std::unique_ptr<SearchToken>>>();
    std::unique_ptr<SearchToken> searchToken = nullptr;
    bool gotToken;
    do {
        searchToken = lexer.nextToken();
        gotToken = !!searchToken;
        if (searchToken)
            searchTokens->push_back(std::move(searchToken));
    } while (gotToken);
    return searchTokens;
}

enum class TestCol {
    Id = 0,
    ItemId,
    PropertyName,
    PropertyValue,
    UpnpClass,
    RefId,
    Last7,
    LastUpdated,
};

const std::map<TestCol, std::pair<std::string, std::string>> testColMap = {
    { TestCol::Id, { "t", "id" } },
    { TestCol::ItemId, { "t", "item_id" } },
    { TestCol::PropertyName, { "t", "property_name" } },
    { TestCol::PropertyValue, { "t", "property_value" } },
    { TestCol::UpnpClass, { "t", "upnp_class" } },
    { TestCol::RefId, { "t", "ref_id" } },
    { TestCol::LastUpdated, { "t", "last_updated" } },
};

const std::vector<std::pair<std::string, TestCol>> testSortMap = {
    { "id", TestCol::Id },
    { UPNP_SEARCH_ID, TestCol::ItemId },
    { META_NAME, TestCol::PropertyName },
    { META_VALUE, TestCol::PropertyValue },
    { UPNP_SEARCH_CLASS, TestCol::UpnpClass },
    { UPNP_SEARCH_REFID, TestCol::RefId },
    { UPNP_SEARCH_LAST_UPDATED, TestCol::LastUpdated },
};

::testing::AssertionResult executeSearchLexerTest(const std::string& input,
    const std::vector<std::pair<std::string, TokenType>>& expectedTokens)
{
    auto tokens = getAllTokens(input);
    if (expectedTokens.size() != tokens->size())
        return ::testing::AssertionFailure() << "Expected " << expectedTokens.size() << " tokens, got " << tokens->size();
    for (unsigned i = 0; i < tokens->size(); i++) {
        if (expectedTokens.at(i).first != tokens->at(i)->getValue())
            return ::testing::AssertionFailure() << "Token " << i << ": expected ["
                                                 << expectedTokens.at(i).first
                                                 << "], got ["
                                                 << tokens->at(i)->getValue() << "]";

        if (expectedTokens.at(i).second != tokens->at(i)->getType())
            return ::testing::AssertionFailure() << "Token type " << i << ": expected ["
                                                 << int(expectedTokens.at(i).second)
                                                 << "], got ["
                                                 << int(tokens->at(i)->getType()) << "]";
    }
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult executeSearchParserTest(const SQLEmitter& emitter, const std::string& input,
    const std::string& expectedOutput, const std::string& expectedRe = "")
{
    try {
        auto parser = SearchParser(emitter, input);
        auto rootNode = parser.parse();
        if (!rootNode)
            return ::testing::AssertionFailure() << "Failed to create AST";

        auto output = rootNode->emit();
        if (output != expectedOutput && !std::regex_match(output, std::regex(expectedRe, std::regex::ECMAScript)))
            return ::testing::AssertionFailure() << "\nExpected [" << expectedOutput << "]\nActual   [" << output << "]\n";

        return ::testing::AssertionSuccess();
    } catch (const std::runtime_error& e) {
        return ::testing::AssertionFailure() << e.what();
    }
}

::testing::AssertionResult executeSortParserTest(const std::string& input,
    const std::string& expectedOutput)
{
    try {
        auto columnMapper = std::make_shared<EnumColumnMapper<TestCol>>('_', '_', "t", "TestTable", testSortMap, testColMap);
        auto parser = SortParser(columnMapper, input);
        auto output = parser.parse();
        if (output.empty())
            return ::testing::AssertionFailure() << "Failed to parse";

        if (output != expectedOutput)
            return ::testing::AssertionFailure() << "\nExpected [" << expectedOutput << "]\nActual   [" << output << "]\n";

        return ::testing::AssertionSuccess();
    } catch (const std::runtime_error& e) {
        return ::testing::AssertionFailure() << e.what();
    }
}

TEST(SearchLexer, OneSimpleTokenRecognized)
{
    auto tokens = getAllTokens("=");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::COMPAREOP, "="), *tokens->at(0));

    tokens = getAllTokens("!=");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::COMPAREOP, "!="), *tokens->at(0));

    tokens = getAllTokens(">");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::COMPAREOP, ">"), *tokens->at(0));

    tokens = getAllTokens("(");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::LPAREN, "("), *tokens->at(0));
}

TEST(SearchLexer, OneComplexTokenRecognized)
{
    auto tokens = getAllTokens("\"");
    EXPECT_EQ(1, tokens->size());
    EXPECT_STREQ("\"", tokens->at(0)->getValue().c_str());
    EXPECT_EQ(1, tokens->at(0)->getValue().length());
    EXPECT_EQ(TokenType::DQUOTE, tokens->at(0)->getType());

    tokens = getAllTokens("true");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::BOOLVAL, "true"), *tokens->at(0));

    tokens = getAllTokens("FALSE");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::BOOLVAL, "FALSE"), *tokens->at(0));

    tokens = getAllTokens("and");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::AND, "and"), *tokens->at(0));

    tokens = getAllTokens("OR");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::OR, "OR"), *tokens->at(0));

    tokens = getAllTokens("exists");
    EXPECT_EQ(1, tokens->size());
    EXPECT_EQ(SearchToken(TokenType::EXISTS, "exists"), *tokens->at(0));

    tokens = getAllTokens("@id");
    EXPECT_EQ(1, tokens->size());
    EXPECT_STREQ("@id", tokens->at(0)->getValue().c_str());
    EXPECT_EQ(TokenType::PROPERTY, tokens->at(0)->getType());

    tokens = getAllTokens("res@size");
    EXPECT_EQ(1, tokens->size());
    EXPECT_STREQ("res@size", tokens->at(0)->getValue().c_str());
    EXPECT_EQ(TokenType::PROPERTY, tokens->at(0)->getType());

    tokens = getAllTokens("dc:title");
    EXPECT_EQ(1, tokens->size());
    EXPECT_STREQ("dc:title", tokens->at(0)->getValue().c_str());
    EXPECT_EQ(TokenType::PROPERTY, tokens->at(0)->getType());
}

TEST(SearchLexer, MultipleTokens)
{
    std::string input = "x=a";
    std::vector<std::pair<std::string, TokenType>> expectedTokens {
        { "x", TokenType::PROPERTY },
        { "=", TokenType::COMPAREOP },
        { "a", TokenType::PROPERTY }
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "x = a";
    expectedTokens = {
        { "x", TokenType::PROPERTY },
        { "=", TokenType::COMPAREOP },
        { "a", TokenType::PROPERTY }
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "xyz=abc";
    expectedTokens = {
        { "xyz", TokenType::PROPERTY },
        { "=", TokenType::COMPAREOP },
        { "abc", TokenType::PROPERTY }
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "xyz=abc fg >= hi";
    expectedTokens = {
        { "xyz", TokenType::PROPERTY },
        { "=", TokenType::COMPAREOP },
        { "abc", TokenType::PROPERTY },
        { "fg", TokenType::PROPERTY },
        { ">=", TokenType::COMPAREOP },
        { "hi", TokenType::PROPERTY }
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "x=\"a\"";
    expectedTokens = {
        { "x", TokenType::PROPERTY },
        { "=", TokenType::COMPAREOP },
        { "\"", TokenType::DQUOTE },
        { "a", TokenType::ESCAPEDSTRING },
        { "\"", TokenType::DQUOTE }
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = "dc:creator = \"Kyuss\"";
    expectedTokens = {
        { "dc:creator", TokenType::PROPERTY },
        { "=", TokenType::COMPAREOP },
        { "\"", TokenType::DQUOTE },
        { "Kyuss", TokenType::ESCAPEDSTRING },
        { "\"", TokenType::DQUOTE }
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = R"(dc:creator = "some band with \"a double-quote")";
    expectedTokens = {
        { "dc:creator", TokenType::PROPERTY },
        { "=", TokenType::COMPAREOP },
        { "\"", TokenType::DQUOTE },
        { "some band with \"a double-quote", TokenType::ESCAPEDSTRING },
        { "\"", TokenType::DQUOTE }
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = R"(dc:creator = "some band with \"a double-quote\"")";
    expectedTokens = {
        { "dc:creator", TokenType::PROPERTY },
        { "=", TokenType::COMPAREOP },
        { "\"", TokenType::DQUOTE },
        { "some band with \"a double-quote\"", TokenType::ESCAPEDSTRING },
        { "\"", TokenType::DQUOTE }
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = R"(upnp:class derivedfrom "object.item.audioItem" and upnp:artist="King Krule")";
    expectedTokens = {
        { "upnp:class", TokenType::PROPERTY },
        { "derivedfrom", TokenType::STRINGOP },
        { "\"", TokenType::DQUOTE },
        { "object.item.audioItem", TokenType::ESCAPEDSTRING },
        { "\"", TokenType::DQUOTE },
        { "and", TokenType::AND },
        { "upnp:artist", TokenType::PROPERTY },
        { "=", TokenType::COMPAREOP },
        { "\"", TokenType::DQUOTE },
        { "King Krule", TokenType::ESCAPEDSTRING },
        { "\"", TokenType::DQUOTE },
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));

    input = R"(upnp:class derivedfrom "object.item.audioItem" and (upnp:artist="King Krule" or dc:title="Heartattack and Vine"))";
    expectedTokens = {
        { "upnp:class", TokenType::PROPERTY },
        { "derivedfrom", TokenType::STRINGOP },
        { "\"", TokenType::DQUOTE },
        { "object.item.audioItem", TokenType::ESCAPEDSTRING },
        { "\"", TokenType::DQUOTE },
        { "and", TokenType::AND },
        { "(", TokenType::LPAREN },
        { "upnp:artist", TokenType::PROPERTY },
        { "=", TokenType::COMPAREOP },
        { "\"", TokenType::DQUOTE },
        { "King Krule", TokenType::ESCAPEDSTRING },
        { "\"", TokenType::DQUOTE },
        { "or", TokenType::OR },
        { "dc:title", TokenType::PROPERTY },
        { "=", TokenType::COMPAREOP },
        { "\"", TokenType::DQUOTE },
        { "Heartattack and Vine", TokenType::ESCAPEDSTRING },
        { "\"", TokenType::DQUOTE },
        { ")", TokenType::RPAREN },
    };
    EXPECT_TRUE(executeSearchLexerTest(input, expectedTokens));
}

TEST(SearchParser, SimpleSearchCriteriaUsingEqualsOperator)
{
    auto columnMapper = std::make_shared<EnumColumnMapper<TestCol>>('_', '_', "t", "TestTable", testSortMap, testColMap);
    DefaultSQLEmitter sqlEmitter(columnMapper, columnMapper, columnMapper);
    // equalsOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
        "dc:title=\"Hospital Roll Call\"",
        "(_t_._property_name_='dc:title' AND LOWER(_t_._property_value_)=LOWER('Hospital Roll Call'))"));

    // equalsOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
        "upnp:album=\"Scraps At Midnight\"",
        "(_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_)=LOWER('Scraps At Midnight'))"));

    // equalsOpExpr or equalsOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
        "upnp:album=\"Scraps At Midnight\" or dc:title=\"Hospital Roll Call\"",
        "(_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_)=LOWER('Scraps At Midnight')) OR (_t_._property_name_='dc:title' AND LOWER(_t_._property_value_)=LOWER('Hospital Roll Call'))"));

    // equalsOpExpr or equalsOpExpr or equalsOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
        "upnp:album=\"Scraps At Midnight\" or dc:title=\"Hospital Roll Call\" or upnp:artist=\"Deafheaven\"",
        "(_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_)=LOWER('Scraps At Midnight')) OR (_t_._property_name_='dc:title' AND LOWER(_t_._property_value_)=LOWER('Hospital Roll Call')) OR (_t_._property_name_='upnp:artist' AND LOWER(_t_._property_value_)=LOWER('Deafheaven'))"));
}

TEST(SearchParser, SearchCriteriaUsingEqualsOperatorParenthesesForSqlite)
{
    auto columnMapper = std::make_shared<EnumColumnMapper<TestCol>>('_', '_', "t", "TestTable", testSortMap, testColMap);
    DefaultSQLEmitter sqlEmitter(columnMapper, columnMapper, columnMapper);
    // (equalsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
        "(upnp:album=\"Scraps At Midnight\")",
        "((_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_)=LOWER('Scraps At Midnight')))"));

    // (equalsOpExpr or equalsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
        "(upnp:album=\"Scraps At Midnight\" or dc:title=\"Hospital Roll Call\")",
        "((_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_)=LOWER('Scraps At Midnight')) OR (_t_._property_name_='dc:title' AND LOWER(_t_._property_value_)=LOWER('Hospital Roll Call')))"));

    // (equalsOpExpr or equalsOpExpr) or equalsOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
        "(upnp:album=\"Scraps At Midnight\" or dc:title=\"Hospital Roll Call\") or upnp:artist=\"Deafheaven\"",
        "((_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_)=LOWER('Scraps At Midnight')) OR (_t_._property_name_='dc:title' AND LOWER(_t_._property_value_)=LOWER('Hospital Roll Call'))) OR (_t_._property_name_='upnp:artist' AND LOWER(_t_._property_value_)=LOWER('Deafheaven'))"));

    // equalsOpExpr or (equalsOpExpr or equalsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
        "upnp:album=\"Scraps At Midnight\" or (dc:title=\"Hospital Roll Call\" or upnp:artist=\"Deafheaven\")",
        "(_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_)=LOWER('Scraps At Midnight')) OR ((_t_._property_name_='dc:title' AND LOWER(_t_._property_value_)=LOWER('Hospital Roll Call')) OR (_t_._property_name_='upnp:artist' AND LOWER(_t_._property_value_)=LOWER('Deafheaven')))"));

    // equalsOpExpr and (equalsOpExpr or equalsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
        "upnp:album=\"Scraps At Midnight\" and (dc:title=\"Hospital Roll Call\" or upnp:artist=\"Deafheaven\")",
        "(_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_)=LOWER('Scraps At Midnight')) AND ((_t_._property_name_='dc:title' AND LOWER(_t_._property_value_)=LOWER('Hospital Roll Call')) OR (_t_._property_name_='upnp:artist' AND LOWER(_t_._property_value_)=LOWER('Deafheaven')))"));

    // equalsOpExpr and (equalsOpExpr or equalsOpExpr or equalsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
        "upnp:album=\"Scraps At Midnight\" and (dc:title=\"Hospital Roll Call\" or upnp:artist=\"Deafheaven\" or upnp:artist=\"Pavement\")",
        "(_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_)=LOWER('Scraps At Midnight')) AND ((_t_._property_name_='dc:title' AND LOWER(_t_._property_value_)=LOWER('Hospital Roll Call')) OR (_t_._property_name_='upnp:artist' AND LOWER(_t_._property_value_)=LOWER('Deafheaven')) OR (_t_._property_name_='upnp:artist' AND LOWER(_t_._property_value_)=LOWER('Pavement')))"));

    // (equalsOpExpr or equalsOpExpr or equalsOpExpr) and equalsOpExpr and equalsOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter,
        "(dc:title=\"Hospital Roll Call\" or upnp:artist=\"Deafheaven\" or upnp:artist=\"Pavement\") and upnp:album=\"Nevermind\" and upnp:album=\"Sunbather\"",
        "((_t_._property_name_='dc:title' AND LOWER(_t_._property_value_)=LOWER('Hospital Roll Call')) OR (_t_._property_name_='upnp:artist' AND LOWER(_t_._property_value_)=LOWER('Deafheaven')) OR (_t_._property_name_='upnp:artist' AND LOWER(_t_._property_value_)=LOWER('Pavement'))) AND (_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_)=LOWER('Nevermind')) AND (_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_)=LOWER('Sunbather'))"));
}

TEST(SearchParser, SearchCriteriaUsingContainsOperator)
{
    auto columnMapper = std::make_shared<EnumColumnMapper<TestCol>>('_', '_', "t", "TestTable", testSortMap, testColMap);
    DefaultSQLEmitter sqlEmitter(columnMapper, columnMapper, columnMapper);
    // (containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album contains \"Midnight\"", "(_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_) LIKE LOWER('%Midnight%'))"));

    // (containsOpExpr or containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album contains \"Midnight\" OR upnp:artist contains \"HEAVE\"",
        "(_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_) LIKE LOWER('%Midnight%')) OR (_t_._property_name_='upnp:artist' AND LOWER(_t_._property_value_) LIKE LOWER('%HEAVE%'))"));
}

TEST(SearchParser, SearchCriteriaUsingDoesNotContainOperator)
{
    auto columnMapper = std::make_shared<EnumColumnMapper<TestCol>>('_', '_', "t", "TestTable", testSortMap, testColMap);
    DefaultSQLEmitter sqlEmitter(columnMapper, columnMapper, columnMapper);
    // (containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album doesnotcontain \"Midnight\"",
        "(_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_) NOT LIKE LOWER('%Midnight%'))"));

    // (containsOpExpr or containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album doesNotContain \"Midnight\" or upnp:artist doesnotcontain \"HEAVE\"",
        "(_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_) NOT LIKE LOWER('%Midnight%')) OR (_t_._property_name_='upnp:artist' AND LOWER(_t_._property_value_) NOT LIKE LOWER('%HEAVE%'))"));
}

TEST(SearchParser, SearchCriteriaUsingStartsWithOperator)
{
    auto columnMapper = std::make_shared<EnumColumnMapper<TestCol>>('_', '_', "t", "TestTable", testSortMap, testColMap);
    DefaultSQLEmitter sqlEmitter(columnMapper, columnMapper, columnMapper);
    // (containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album startswith \"Midnight\"", "(_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_) LIKE LOWER('Midnight%'))"));

    // (containsOpExpr or containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album startsWith \"Midnight\" or upnp:artist startswith \"HEAVE\"",
        "(_t_._property_name_='upnp:album' AND LOWER(_t_._property_value_) LIKE LOWER('Midnight%')) OR (_t_._property_name_='upnp:artist' AND LOWER(_t_._property_value_) LIKE LOWER('HEAVE%'))"));
}

TEST(SearchParser, SearchCriteriaUsingExistsOperator)
{
    auto columnMapper = std::make_shared<EnumColumnMapper<TestCol>>('_', '_', "t", "TestTable", testSortMap, testColMap);
    DefaultSQLEmitter sqlEmitter(columnMapper, columnMapper, columnMapper);
    // (containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album exists true",
        "(_t_._property_name_='upnp:album' AND _t_._property_value_ IS NOT NULL)"));

    // (containsOpExpr or containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:album exists true or upnp:artist exists false",
        "(_t_._property_name_='upnp:album' AND _t_._property_value_ IS NOT NULL) OR (_t_._property_name_='upnp:artist' AND _t_._property_value_ IS NULL)"));
}

TEST(SearchParser, SearchCriteriaWithExtendsOperator)
{
    auto columnMapper = std::make_shared<EnumColumnMapper<TestCol>>('_', '_', "t", "TestTable", testSortMap, testColMap);
    DefaultSQLEmitter sqlEmitter(columnMapper, columnMapper, columnMapper);
    // derivedfromOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:class derivedfrom \"object.item.audioItem\"",
        "(LOWER(_t_._upnp_class_) LIKE LOWER('object.item.audioItem%'))"));

    // derivedfromOpExpr and (containsOpExpr or containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:class derivedfrom \"object.item.audioItem\" and (dc:title contains \"britain\" or dc:creator contains \"britain\"",
        "(LOWER(_t_._upnp_class_) LIKE LOWER('object.item.audioItem%')) AND ((_t_._property_name_='dc:title' AND LOWER(_t_._property_value_) LIKE LOWER('%britain%')) OR (_t_._property_name_='dc:creator' AND LOWER(_t_._property_value_) LIKE LOWER('%britain%')))"));

    // derivedFromOpExpr and (containsOpExpr or containsOpExpr)
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:class derivedFrom \"object.item.audioItem\" and (dc:title contains \"britain\" or dc:creator contains \"britain\"",
        "(LOWER(_t_._upnp_class_) LIKE LOWER('object.item.audioItem%')) AND ((_t_._property_name_='dc:title' AND LOWER(_t_._property_value_) LIKE LOWER('%britain%')) OR (_t_._property_name_='dc:creator' AND LOWER(_t_._property_value_) LIKE LOWER('%britain%')))"));
}

TEST(SearchParser, SearchCriteriaWindowMedia)
{
    auto columnMapper = std::make_shared<EnumColumnMapper<TestCol>>('_', '_', "t", "TestTable", testSortMap, testColMap);
    DefaultSQLEmitter sqlEmitter(columnMapper, columnMapper, columnMapper);
    // derivedfromOpExpr
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:class derivedfrom \"object.item.videoItem\" and @refID exists false",
        "(LOWER(_t_._upnp_class_) LIKE LOWER('object.item.videoItem%')) AND (_t_._ref_id_ IS NULL)"));
}

TEST(SearchParser, SearchCriteriaDynamic)
{
    auto columnMapper = std::make_shared<EnumColumnMapper<TestCol>>('_', '_', "t", "TestTable", testSortMap, testColMap);
    DefaultSQLEmitter sqlEmitter(columnMapper, columnMapper, columnMapper);
    EXPECT_TRUE(executeSearchParserTest(sqlEmitter, "upnp:class derivedfrom \"object.item\" and last_updated > \"@last7\"",
        "(LOWER(_t_._upnp_class_) LIKE LOWER('object.item%')) AND (_t_._last_updated_ > [0-9]+))",
        R"(\(LOWER\(_t_\._upnp_class_\) LIKE LOWER\('object\.item%'\)\) AND \(_t_\._last_updated_ > [0-9]+\))")); // regular expression because last7 is dynamic
}

TEST(SortParser, SortCriteria)
{
    EXPECT_TRUE(executeSortParserTest("+id,-name,+value",
        "_t_._id_ ASC, _t_._property_name_ DESC, _t_._property_value_ ASC"));
}

TEST(SortParser, SortCriteriaNoDir)
{
    EXPECT_TRUE(executeSortParserTest("+id,name,+value",
        "_t_._id_ ASC, _t_._property_name_ ASC, _t_._property_value_ ASC"));
}

TEST(SortParser, SortCriteriaError)
{
    EXPECT_TRUE(executeSortParserTest("+id,nme,+value",
        "_t_._id_ ASC, _t_._property_value_ ASC"));
}
