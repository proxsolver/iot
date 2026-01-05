/**
 * SEO-Optimized Blog Content Agent for Technical Tutorials
 * Specialized in IoT, Arduino, LoRaWAN, and Computer Vision content
 */

class SEOTechnicalBlogAgent {
  constructor() {
    this.keywords = {
      primary: [],
      secondary: [],
      longTail: []
    };
    this.contentMetrics = {
      wordCount: 0,
      keywordDensity: 0,
      readabilityScore: 0,
      headingCount: 0
    };
  }

  /**
   * Analyze content and provide SEO recommendations
   * @param {string} content - The blog post content
   * @param {object} targetKeywords - Target keywords for optimization
   * @returns {object} SEO analysis and recommendations
   */
  analyzeContent(content, targetKeywords) {
    const analysis = {
      title: this.analyzeTitle(content),
      headings: this.analyzeHeadings(content),
      keywords: this.analyzeKeywords(content, targetKeywords),
      readability: this.analyzeReadability(content),
      meta: this.generateMetaSuggestions(content, targetKeywords),
      structure: this.analyzeStructure(content),
      technicalAccuracy: this.assessTechnicalAccuracy(content),
      recommendations: []
    };

    analysis.recommendations = this.generateRecommendations(analysis);
    return analysis;
  }

  /**
   * Analyze title for SEO optimization
   */
  analyzeTitle(content) {
    const titleMatch = content.match(/^#\s+(.+)$/m);
    if (!titleMatch) {
      return {
        status: 'missing',
        suggestion: 'Add a clear, descriptive title with primary keyword'
      };
    }

    const title = titleMatch[1];
    const length = title.length;
    const wordCount = title.split(' ').length;

    return {
      title,
      length,
      wordCount,
      status: length >= 50 && length <= 60 ? 'optimal' : 'suboptimal',
      suggestion: this.getTitleSuggestion(title)
    };
  }

  getTitleSuggestion(title) {
    if (title.length < 50) {
      return 'Title is too short. Add descriptive keywords (aim for 50-60 characters)';
    }
    if (title.length > 60) {
      return 'Title may be truncated in search results. Condense to 50-60 characters';
    }
    return 'Title length is optimal for SEO';
  }

  /**
   * Analyze heading structure (H1, H2, H3)
   */
  analyzeHeadings(content) {
    const headings = {
      h1: [],
      h2: [],
      h3: []
    };

    const lines = content.split('\n');
    lines.forEach(line => {
      const match = line.match(/^(#{1,3})\s+(.+)$/);
      if (match) {
        const level = match[1].length;
        const text = match[2];
        if (level === 1) headings.h1.push(text);
        if (level === 2) headings.h2.push(text);
        if (level === 3) headings.h3.push(text);
      }
    });

    return {
      count: headings.h1.length + headings.h2.length + headings.h3.length,
      h1: headings.h1,
      h2: headings.h2,
      h3: headings.h3,
      status: this.assessHeadingStructure(headings),
      suggestions: this.getHeadingSuggestions(headings)
    };
  }

  assessHeadingStructure(headings) {
    if (headings.h1.length === 0) return 'missing H1';
    if (headings.h1.length > 1) return 'multiple H1 (should have only one)';
    if (headings.h2.length < 2) return 'needs more H2 sections';
    return 'optimal';
  }

  getHeadingSuggestions(headings) {
    const suggestions = [];
    if (headings.h1.length === 0) {
      suggestions.push('Add a main H1 heading with your primary keyword');
    } else if (headings.h1.length > 1) {
      suggestions.push('Use only one H1 heading; convert others to H2 or H3');
    }
    if (headings.h2.length < 2) {
      suggestions.push('Add at least 2-3 H2 headings to structure your content');
    }
    suggestions.push('Include keywords naturally in headings');
    suggestions.push('Use descriptive, scannable headings that summarize content');
    return suggestions;
  }

  /**
   * Analyze keyword usage and density
   */
  analyzeKeywords(content, targetKeywords) {
    const words = content.toLowerCase().split(/\s+/);
    const totalWords = words.length;
    const keywordAnalysis = {};

    Object.keys(targetKeywords).forEach(category => {
      keywordAnalysis[category] = targetKeywords[category].map(keyword => {
        const regex = new RegExp(keyword.toLowerCase(), 'gi');
        const matches = content.match(regex) || [];
        const density = (matches.length / totalWords) * 100;

        return {
          keyword,
          count: matches.length,
          density: density.toFixed(2),
          status: density >= 0.5 && density <= 2.5 ? 'optimal' : density < 0.5 ? 'low' : 'high'
        };
      });
    });

    return {
      totalWords,
      keywordAnalysis,
      suggestions: this.getKeywordSuggestions(keywordAnalysis)
    };
  }

  getKeywordSuggestions(keywordAnalysis) {
    const suggestions = [];
    Object.values(keywordAnalysis).forEach(keywords => {
      keywords.forEach(({ keyword, density, status }) => {
        if (status === 'low') {
          suggestions.push(`Keyword "${keyword}" density is low (${density}%). Aim for 0.5-2.5%`);
        } else if (status === 'high') {
          suggestions.push(`Keyword "${keyword}" is overused (${density}%). Reduce to avoid keyword stuffing`);
        }
      });
    });
    return suggestions;
  }

  /**
   * Analyze content readability
   */
  analyzeReadability(content) {
    const sentences = content.split(/[.!?]+/).filter(s => s.trim().length > 0);
    const words = content.split(/\s+/);
    const syllables = this.countSyllables(content);

    // Flesch Reading Ease Score
    const avgSentenceLength = words.length / sentences.length;
    const avgSyllablesPerWord = syllables / words.length;
    const fleschScore = 206.835 - (1.015 * avgSentenceLength) - (84.6 * avgSyllablesPerWord);

    return {
      fleschScore: fleschScore.toFixed(1),
      avgSentenceLength: avgSentenceLength.toFixed(1),
      avgWordsPerSentence: Math.round(avgSentenceLength),
      status: fleschScore >= 60 ? 'readable' : fleschScore >= 30 ? 'moderate' : 'difficult',
      suggestions: this.getReadabilitySuggestions(fleschScore, avgSentenceLength)
    };
  }

  countSyllables(text) {
    const words = text.toLowerCase().split(/\s+/);
    let syllableCount = 0;

    words.forEach(word => {
      word = word.replace(/[^a-z]/g, '');
      if (word.length <= 3) {
        syllableCount += 1;
      } else {
        const vowelGroups = word.match(/[aeiouy]+/g);
        syllableCount += vowelGroups ? vowelGroups.length : 1;
      }
    });

    return syllableCount;
  }

  getReadabilitySuggestions(fleschScore, avgSentenceLength) {
    const suggestions = [];
    if (fleschScore < 60) {
      suggestions.push('Content is difficult to read. Use shorter sentences and simpler words');
    }
    if (avgSentenceLength > 25) {
      suggestions.push('Sentences are too long. Aim for 15-20 words per sentence');
    }
    suggestions.push('Use bullet points and numbered lists to break up complex information');
    suggestions.push('Define technical terms and provide examples');
    suggestions.push('Use active voice instead of passive voice');
    return suggestions;
  }

  /**
   * Generate meta description and title suggestions
   */
  generateMetaSuggestions(content, targetKeywords) {
    const primaryKeyword = targetKeywords.primary[0] || '';
    const firstParagraph = content.match(/^#+\s+.+?\n+(.+?)(?=\n#|\n\n|$)/s);

    let description = '';
    if (firstParagraph) {
      description = firstParagraph[1]
        .replace(/[#*`\[\]]/g, '')
        .trim()
        .substring(0, 155);
    }

    return {
      metaTitle: this.generateMetaTitle(content, primaryKeyword),
      metaDescription: this.generateMetaDescription(description, primaryKeyword),
      suggestions: [
        'Keep meta description between 150-160 characters',
        'Include primary keyword naturally in meta description',
        'Make it compelling to increase click-through rate',
        'Include a call-to-action or value proposition'
      ]
    };
  }

  generateMetaTitle(content, primaryKeyword) {
    const titleMatch = content.match(/^#\s+(.+)$/m);
    const title = titleMatch ? titleMatch[1] : 'Untitled';

    if (!title.includes(primaryKeyword)) {
      return `${title} - ${primaryKeyword} Tutorial`;
    }
    return title;
  }

  generateMetaDescription(description, primaryKeyword) {
    if (description.length < 120) {
      return `${description} Learn ${primaryKeyword} with this step-by-step tutorial.`;
    }
    if (description.length > 160) {
      return description.substring(0, 157) + '...';
    }
    return description;
  }

  /**
   * Analyze overall content structure
   */
  analyzeStructure(content) {
    const hasIntro = /^\s*#+.+(?:\n+.+?){3,}/m.test(content);
    const hasCodeBlocks = /```[\s\S]*?```/.test(content);
    const hasLists = /^\s*[-*+]\s+/m.test(content) || /^\s*\d+\.\s+/m.test(content);
    const hasImages = /!\[.+\]\(.+\)/.test(content);
    const hasConclusion = /(?:conclusion|summary|final|wrap-up)/i.test(content);

    const paragraphs = content.split(/\n\n+/).filter(p => p.trim().length > 0);
    const avgParagraphLength = paragraphs.reduce((sum, p) => sum + p.split(/\s+/).length, 0) / paragraphs.length;

    return {
      hasIntroduction: hasIntro,
      hasCodeBlocks: hasCodeBlocks,
      hasLists: hasLists,
      hasImages: hasImages,
      hasConclusion: hasConclusion,
      paragraphCount: paragraphs.length,
      avgParagraphLength: Math.round(avgParagraphLength),
      suggestions: this.getStructureSuggestions({ hasIntro, hasCodeBlocks, hasLists, hasImages, hasConclusion })
    };
  }

  getStructureSuggestions(structure) {
    const suggestions = [];
    if (!structure.hasIntroduction) {
      suggestions.push('Add a clear introduction that hooks readers and outlines what they will learn');
    }
    if (!structure.hasCodeBlocks) {
      suggestions.push('Include code examples with syntax highlighting for better engagement');
    }
    if (!structure.hasLists) {
      suggestions.push('Use bullet points or numbered lists to break up dense content');
    }
    if (!structure.hasImages) {
      suggestions.push('Add relevant images, diagrams, or screenshots to illustrate concepts');
    }
    if (!structure.hasConclusion) {
      suggestions.push('Add a conclusion or summary section to reinforce key takeaways');
    }
    return suggestions;
  }

  /**
   * Assess technical accuracy (basic check)
   */
  assessTechnicalAccuracy(content) {
    const technicalElements = {
      hasCodeExamples: /```[\s\S]*?```/.test(content),
      hasVersionInfo: /version?\s*\d+\.\d+/i.test(content),
      hasErrorHandling: /error|exception|try|catch|debug/i.test(content),
      hasDependencies: /install|require|import|include|npm|pip|library/i.test(content),
      hasTroubleshooting: /troubleshoot|fix|issue|problem|solution/i.test(content)
    };

    return {
      technicalElements,
      score: Object.values(technicalElements).filter(Boolean).length,
      suggestions: [
        'Include specific version numbers for hardware/software',
        'Add error handling and common pitfalls section',
        'List all required dependencies and installation steps',
        'Include troubleshooting section for common issues',
        'Provide links to official documentation'
      ]
    };
  }

  /**
   * Generate prioritized recommendations
   */
  generateRecommendations(analysis) {
    const recommendations = [];
    const priority = ['high', 'medium', 'low'];

    if (analysis.title.status !== 'optimal') {
      recommendations.push({
        priority: 'high',
        category: 'Title',
        issue: analysis.title.status,
        suggestion: analysis.title.suggestion
      });
    }

    if (analysis.headings.status !== 'optimal') {
      recommendations.push({
        priority: 'high',
        category: 'Headings',
        issue: analysis.headings.status,
        suggestion: analysis.headings.suggestions.join(' ')
      });
    }

    if (analysis.readability.status === 'difficult') {
      recommendations.push({
        priority: 'medium',
        category: 'Readability',
        issue: 'Content is difficult to read',
        suggestion: analysis.readability.suggestions.join(' ')
      });
    }

    return recommendations.sort((a, b) => {
      return priority.indexOf(a.priority) - priority.indexOf(b.priority);
    });
  }

  /**
   * Generate engaging title variants
   */
  generateTitleVariants(topic, primaryKeyword, targetAudience) {
    const templates = [
      `How to ${topic}: A Complete ${primaryKeyword} Tutorial`,
      `${topic} Made Easy: Step-by-Step ${primaryKeyword} Guide`,
      `Building ${topic} with ${primaryKeyword}: From Zero to Hero`,
      `Master ${primaryKeyword}: Build Your Own ${topic}`,
      `${topic} Tutorial: Complete ${primaryKeyword} Project Guide`,
      `Build a ${topic} in ${Math.floor(Math.random() * 30) + 10} Minutes with ${primaryKeyword}`,
      `The Ultimate ${primaryKeyword} Tutorial: ${topic} Explained`,
      `${primaryKeyword} Project: How I Built ${topic}`,
      `Beginner's Guide to ${topic} Using ${primaryKeyword}`,
      `${topic} with ${primaryKeyword}: A Practical Tutorial`
    ];

    return templates.map(template => ({
      title: template,
      length: template.length,
      wordCount: template.split(' ').length
    }));
  }

  /**
   * Generate content summary for social media
   */
  generateSocialSummary(content, maxLength = 280) {
    const sentences = content.split(/[.!?]+/).filter(s => s.trim().length > 0);
    let summary = '';

    for (const sentence of sentences) {
      if ((summary + sentence).length > maxLength) {
        break;
      }
      summary += sentence.trim() + '. ';
    }

    return summary.trim() || 'Check out this tutorial! ' + content.substring(0, 100) + '...';
  }
}

module.exports = SEOTechnicalBlogAgent;
